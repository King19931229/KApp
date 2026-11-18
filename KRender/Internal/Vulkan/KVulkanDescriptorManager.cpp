#include "KVulkanDescriptorManager.h"
#include <algorithm>
#include <limits>
#include <cstdio>

KVulkanDescriptorManager::~KVulkanDescriptorManager()
{
	ASSERT_RESULT(m_Tickets.empty());
}

KVulkanDescriptorTicket* KVulkanDescriptorManager::Create(VkDescriptorSetLayout layout,
	const std::vector<VkDescriptorSetLayoutBinding>& bindings,
	uint32_t threadCount, uint32_t blockSize, bool trim)
{
	if (layout == VK_NULL_HANDLE || !threadCount || !blockSize) return nullptr;
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (const VkDescriptorSetLayoutBinding& binding : bindings)
	{
		auto it = std::find_if(poolSizes.begin(), poolSizes.end(), [&binding](const VkDescriptorPoolSize& size)
		{
			return size.type == binding.descriptorType;
		});
		uint64_t count = binding.descriptorCount;
		if (it != poolSizes.end()) count += it->descriptorCount;
		if (count > (std::numeric_limits<uint32_t>::max)() / blockSize) return nullptr;
		if (it == poolSizes.end()) poolSizes.push_back({ binding.descriptorType, (uint32_t)count });
		else it->descriptorCount = (uint32_t)count;
	}

	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	m_Tickets.emplace_back(threadCount);
	KVulkanDescriptorTicket& ticket = m_Tickets.back();
	ticket.layout = layout;
	ticket.poolSizes = std::move(poolSizes);
	ticket.blockSize = blockSize;
	ticket.trim = trim;
	return &ticket;
}

KVulkanDescriptorTicket::ThreadResources& KVulkanDescriptorManager::GetThreadResources(KVulkanDescriptorTicket* ticket, uint32_t threadIndex)
{
	ASSERT_RESULT(ticket && !ticket->retired && threadIndex < ticket->threads.size());
	// 每个槽只由对应的录制线程访问，帧回收在录制任务结束后执行。
	auto& resources = ticket->threads[threadIndex];
	if (!resources)
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		resources.reset(KNEW KVulkanDescriptorTicket::ThreadResources());
		KVulkanDescriptorPool& pool = resources->pool;
		ASSERT_RESULT(pool.Init(ticket->layout, ticket->poolSizes, ticket->blockSize, ticket->trim));
		pool.SetArenaProvider([this, ticket, threadIndex](VkCommandPool commandPool)
		{
			return GetArena(ticket, threadIndex, commandPool);
		});
		if (!ticket->name.empty())
		{
			pool.SetDebugName(ticket->name + "_Thread" + std::to_string(threadIndex));
		}
		ticket->activeThreads.push_back(threadIndex);
	}
	return *resources;
}

std::shared_ptr<KVulkanDescriptorArena> KVulkanDescriptorManager::GetArena(KVulkanDescriptorTicket* ticket, uint32_t threadIndex, VkCommandPool commandPool)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	auto& owner = m_CommandPools.emplace(commandPool, CommandPoolResources{ threadIndex }).first->second;
	if (owner.threadIndex != threadIndex)
	{
		std::fprintf(stderr, "Vulkan descriptor ownership mismatch: pool=%llu thread=%u expected=%u\n",
			(unsigned long long)commandPool, threadIndex, owner.threadIndex);
		assert(false && "command pool used by a different recording thread");
		return nullptr;
	}
	auto range = owner.arenas.equal_range(ticket->layout);
	for (auto it = range.first; it != range.second;)
	{
		auto arena = it->second.lock();
		if (!arena)
		{
			it = owner.arenas.erase(it);
			continue;
		}
		if (arena->Matches(ticket->poolSizes)) return arena;
		++it;
	}
	auto arena = std::make_shared<KVulkanDescriptorArena>(ticket->poolSizes);
	owner.arenas.emplace(ticket->layout, arena);
	return arena;
}

KVulkanDescriptorPoolAllocatedSetPtr KVulkanDescriptorManager::Alloc(KVulkanDescriptorTicket* ticket, uint32_t threadIndex, bool& newSet, VkCommandPool commandPool)
{
	return GetThreadResources(ticket, threadIndex).pool.Alloc(newSet, commandPool);
}

KVulkanDescriptorPoolAllocatedSetPtr KVulkanDescriptorManager::Request(KVulkanDescriptorTicket* ticket, bool& newSet)
{
	ASSERT_RESULT(ticket && ticket->threads.size() == 1 && ticket->blockSize == 1);
	return GetThreadResources(ticket, 0).pool.Request(newSet);
}

KVulkanGraphicsDescriptorWriter& KVulkanDescriptorManager::GetGraphicsWriter(KVulkanDescriptorTicket* ticket, uint32_t threadIndex)
{
	auto& writer = GetThreadResources(ticket, threadIndex).writer;
	if (!writer)
	{
		writer.reset(KNEW KVulkanGraphicsDescriptorWriter());
		ASSERT_RESULT(writer->Init(ticket->poolSizes));
	}
	return *writer;
}

std::vector<VkDescriptorSet> KVulkanDescriptorManager::GetDescriptorSets(const KVulkanDescriptorTicket* ticket) const
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	ASSERT_RESULT(ticket && !ticket->retired);
	std::vector<VkDescriptorSet> sets;
	for (uint32_t threadIndex : ticket->activeThreads)
	{
		ticket->threads[threadIndex]->pool.AppendDescriptorSets(sets);
	}
	return sets;
}

void KVulkanDescriptorManager::DestroyUnusedTickets()
{
	for (auto it = m_Tickets.begin(); it != m_Tickets.end();)
	{
		KVulkanDescriptorTicket& ticket = *it;
		if (!ticket.retired)
		{
			++it;
			continue;
		}
		bool occupied = false;
		for (uint32_t threadIndex : ticket.activeThreads)
		{
			occupied |= ticket.threads[threadIndex]->pool.IsOccupied();
		}
		if (!occupied)
		{
			for (uint32_t threadIndex : ticket.activeThreads)
			{
				ticket.threads[threadIndex]->pool.UnInit();
			}
			it = m_Tickets.erase(it);
		}
		else ++it;
	}
}

void KVulkanDescriptorManager::Destroy(KVulkanDescriptorTicket* ticket)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	ASSERT_RESULT(ticket && !ticket->retired);
	ticket->retired = true;
}

void KVulkanDescriptorManager::Release(uint32_t frameNum)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	for (KVulkanDescriptorTicket& ticket : m_Tickets)
	{
		for (uint32_t threadIndex : ticket.activeThreads)
		{
			ticket.threads[threadIndex]->pool.Release(frameNum);
		}
	}
	DestroyUnusedTickets();
}

void KVulkanDescriptorManager::ReleaseCommandPool(VkCommandPool commandPool)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	auto owner = m_CommandPools.find(commandPool);
	if (owner == m_CommandPools.end()) return;
	const uint32_t threadIndex = owner->second.threadIndex;
	for (KVulkanDescriptorTicket& ticket : m_Tickets)
	{
		if (threadIndex < ticket.threads.size() && ticket.threads[threadIndex])
		{
			ticket.threads[threadIndex]->pool.ReleaseCommandPool(commandPool);
		}
	}
	m_CommandPools.erase(owner);
}

void KVulkanDescriptorManager::SetDebugName(KVulkanDescriptorTicket* ticket, const char* name)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	ticket->name = name;
}

void KVulkanDescriptorManager::UnInit()
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	for (KVulkanDescriptorTicket& ticket : m_Tickets)
	{
		for (uint32_t threadIndex : ticket.activeThreads)
		{
			ticket.threads[threadIndex]->pool.UnInit();
		}
	}
	m_Tickets.clear();
	m_CommandPools.clear();
}
