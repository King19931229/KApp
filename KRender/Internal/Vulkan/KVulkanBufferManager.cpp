#include "KVulkanBufferManager.h"
#include "Internal/KRenderGlobal.h"

KVulkanBufferManager::~KVulkanBufferManager()
{
	ASSERT_RESULT(m_Tickets.empty());
}

KVulkanBufferTicket* KVulkanBufferManager::Create(VkBufferUsageFlags usages, const void* data, uint64_t bufferSize, bool hostVisible)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	m_Tickets.emplace_back();
	KVulkanBufferTicket& ticket = m_Tickets.back();
	ticket.usages = usages;
	ticket.bufferSize = bufferSize;
	ticket.hostVisible = hostVisible;
	if (bufferSize)
	{
		ticket.buffers.emplace_back();
		if (!ticket.buffers.back().buffer.InitDevice(usages, data, bufferSize, hostVisible))
		{
			ticket.buffers.back().buffer.UnInit();
			m_Tickets.pop_back();
			return nullptr;
		}
	}
	return &ticket;
}

KVulkanBufferTicket* KVulkanBufferManager::Create(VkBufferUsageFlags usages, bool hostVisible)
{
	return Create(usages, nullptr, 0, hostVisible);
}

KVulkanBuffer& KVulkanBufferManager::Request(KVulkanBufferTicket* ticket, const void* initialData)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	ASSERT_RESULT(ticket && !ticket->retired);
	const uint32_t frameNum = KRenderGlobal::CurrentFrameNum;
	KVulkanBufferTicket::Allocation* available = nullptr;
	for (auto& allocation : ticket->buffers)
	{
		if (allocation.occupied && allocation.frameNum == frameNum) return allocation.buffer;
		if (!allocation.occupied && !available) available = &allocation;
	}
	if (!available)
	{
		ticket->buffers.emplace_back();
		available = &ticket->buffers.back();
		ASSERT_RESULT(available->buffer.InitDevice(ticket->usages, initialData, ticket->bufferSize, ticket->hostVisible));
		if (!ticket->name.empty())
		{
			available->buffer.SetDebugName((ticket->name + "_" + std::to_string(ticket->buffers.size() - 1)).c_str());
		}
	}
	available->frameNum = frameNum;
	available->occupied = true;
	return available->buffer;
}

KVulkanBuffer* KVulkanBufferManager::RequestUpload(KVulkanBufferTicket* ticket, uint64_t size, uint32_t frameNum)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	if (!ticket || ticket->retired || !size) return nullptr;

	auto available = ticket->buffers.end();
	for (auto it = ticket->buffers.begin(); it != ticket->buffers.end(); ++it)
	{
		if (it->occupied) continue;
		if (it->buffer.GetBufferSize() >= size)
		{
			available = it;
			break;
		}
		if (available == ticket->buffers.end()) available = it;
	}
	if (available == ticket->buffers.end()) available = ticket->buffers.emplace(available);

	KVulkanBuffer& buffer = available->buffer;
	if (buffer.GetBufferSize() < size)
	{
		if (!buffer.InitDevice(ticket->usages, nullptr, size, ticket->hostVisible)) return nullptr;
		if (!ticket->name.empty())
		{
			size_t index = std::distance(ticket->buffers.begin(), available);
			buffer.SetDebugName((ticket->name + "_" + std::to_string(index)).c_str());
		}
	}
	available->frameNum = frameNum;
	available->occupied = true;
	return &buffer;
}

void KVulkanBufferManager::DestroyUnusedTickets()
{
	for (auto it = m_Tickets.begin(); it != m_Tickets.end();)
	{
		KVulkanBufferTicket& ticket = *it;
		bool occupied = false;
		for (auto& allocation : ticket.buffers) occupied |= allocation.occupied;
		if (ticket.retired && !occupied)
		{
			for (auto& allocation : ticket.buffers) allocation.buffer.UnInit();
			it = m_Tickets.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void KVulkanBufferManager::Destroy(KVulkanBufferTicket* ticket)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	ticket->retired = true;
	DestroyUnusedTickets();
}

void KVulkanBufferManager::Release(uint32_t frameNum)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	for (auto& ticket : m_Tickets)
	{
		for (auto& allocation : ticket.buffers)
		{
			if (allocation.occupied && allocation.frameNum == frameNum)
			{
				allocation.occupied = false;
				if (ticket.upload.buffer == &allocation.buffer)
				{
					ticket.upload = {};
				}
			}
		}
	}
	DestroyUnusedTickets();
}

void KVulkanBufferManager::SetDebugName(KVulkanBufferTicket* ticket, const char* name)
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	ticket->name = name;
	size_t index = 0;
	for (auto& allocation : ticket->buffers)
	{
		allocation.buffer.SetDebugName((ticket->name + "_" + std::to_string(index++)).c_str());
	}
}

void KVulkanBufferManager::UnInit()
{
	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
	for (auto& ticket : m_Tickets)
	{
		for (auto& allocation : ticket.buffers) allocation.buffer.UnInit();
	}
	m_Tickets.clear();
}
