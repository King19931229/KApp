#pragma once
#include "KVulkanBuffer.h"
#include <list>
#include <mutex>

struct KVulkanBufferTicket
{
	struct Allocation
	{
		KVulkanBuffer buffer;
		uint32_t frameNum = 0;
		bool occupied = false;
	};
	std::list<Allocation> buffers;
	VkBufferUsageFlags usages = 0;
	uint64_t bufferSize = 0;
	bool hostVisible = false;
	bool retired = false;
	std::string name;
};

class KVulkanBufferManager
{
protected:
	std::list<KVulkanBufferTicket> m_Tickets;
	std::mutex m_Lock;
	void DestroyUnusedTickets();
public:
	~KVulkanBufferManager();
	KVulkanBufferTicket* Create(VkBufferUsageFlags usages, const void* data, uint64_t bufferSize, bool hostVisible);
	KVulkanBuffer& Request(KVulkanBufferTicket* ticket, const void* initialData);
	void Destroy(KVulkanBufferTicket* ticket);
	void Release(uint32_t frameNum);
	void SetDebugName(KVulkanBufferTicket* ticket, const char* name);
	void UnInit();
};
