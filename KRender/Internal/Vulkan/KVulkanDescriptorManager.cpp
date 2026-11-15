#include "KVulkanDescriptorManager.h"
#include <algorithm>
#include <limits>

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

KVulkanDescriptorPool& KVulkanDescriptorManager::GetPool(KVulkanDescriptorTicket* ticket, uint32_t threadIndex)
{
	ASSERT_RESULT(ticket && !ticket->retired && threadIndex < ticket->pools.size());
	KVulkanDescriptorPool& pool = ticket->pools[threadIndex];
	if (!pool.IsInitialized())
	{
		ASSERT_RESULT(pool.Init(ticket->layout, ticket->poolSizes, ticket->blockSize, ticket->trim));
		if (!ticket->name.empty())
		{
			pool.SetDebugName(ticket->name + "_Thread" + std::to_string(threadIndex));
		}
	}
	return pool;
}

KVulkanDescriptorPoolAllocatedSetPtr KVulkanDescriptorManager::Alloc(KVulkanDescriptorTicket* ticket, uint32_t threadIndex, bool& newSet)
{
	return GetPool(ticket, threadIndex).Alloc(newSet);
}

KVulkanDescriptorPoolAllocatedSetPtr KVulkanDescriptorManager::Request(KVulkanDescriptorTicket* ticket, bool& newSet)
{
	ASSERT_RESULT(ticket && ticket->pools.size() == 1 && ticket->blockSize == 1);
	return GetPool(ticket, 0).Request(newSet);
}

KVulkanGraphicsDescriptorWriter& KVulkanDescriptorManager::GetGraphicsWriter(KVulkanDescriptorTicket* ticket, uint32_t threadIndex)
{
	ASSERT_RESULT(ticket && !ticket->retired && threadIndex < ticket->writers.size());
	std::unique_ptr<KVulkanGraphicsDescriptorWriter>& writer = ticket->writers[threadIndex];
	if (!writer)
	{
		writer.reset(KNEW KVulkanGraphicsDescriptorWriter());
		ASSERT_RESULT(writer->Init(ticket->poolSizes));
	}
	return *writer;
}

std::vector<VkDescriptorSet> KVulkanDescriptorManager::GetDescriptorSets(const KVulkanDescriptorTicket* ticket) const
{
	ASSERT_RESULT(ticket && !ticket->retired);
	std::vector<VkDescriptorSet> sets;
	for (const KVulkanDescriptorPool& pool : ticket->pools) pool.AppendDescriptorSets(sets);
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
		for (const KVulkanDescriptorPool& pool : ticket.pools) occupied |= pool.IsOccupied();
		if (!occupied)
		{
			for (KVulkanDescriptorPool& pool : ticket.pools) pool.UnInit();
			it = m_Tickets.erase(it);
		}
		else ++it;
	}
}

void KVulkanDescriptorManager::Destroy(KVulkanDescriptorTicket* ticket)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	ticket->retired = true;
	DestroyUnusedTickets();
}

void KVulkanDescriptorManager::Release(uint32_t frameNum)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	for (KVulkanDescriptorTicket& ticket : m_Tickets)
	{
		for (KVulkanDescriptorPool& pool : ticket.pools) pool.Release(frameNum);
	}
	DestroyUnusedTickets();
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
		for (KVulkanDescriptorPool& pool : ticket.pools) pool.UnInit();
	}
	m_Tickets.clear();
}
