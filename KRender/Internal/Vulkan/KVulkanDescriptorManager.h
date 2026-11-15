#pragma once
#include "KVulkanDescripiorPool.h"
#include <list>
#include <memory>
#include <mutex>

struct KVulkanDescriptorTicket
{
private:
	friend class KVulkanDescriptorManager;
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> poolSizes;
	std::vector<KVulkanDescriptorPool> pools;
	std::vector<std::unique_ptr<KVulkanGraphicsDescriptorWriter>> writers;
	uint32_t blockSize = 0;
	bool trim = false;
	bool retired = false;
	std::string name;

public:
	explicit KVulkanDescriptorTicket(uint32_t threadCount) : pools(threadCount), writers(threadCount) {}
};

class KVulkanDescriptorManager
{
protected:
	std::list<KVulkanDescriptorTicket> m_Tickets;
	std::mutex m_Lock;
	KVulkanDescriptorPool& GetPool(KVulkanDescriptorTicket* ticket, uint32_t threadIndex);
	void DestroyUnusedTickets();
public:
	~KVulkanDescriptorManager();
	KVulkanDescriptorTicket* Create(VkDescriptorSetLayout layout,
		const std::vector<VkDescriptorSetLayoutBinding>& bindings,
		uint32_t threadCount, uint32_t blockSize, bool trim);
	KVulkanDescriptorPoolAllocatedSetPtr Alloc(KVulkanDescriptorTicket* ticket, uint32_t threadIndex, bool& newSet);
	KVulkanDescriptorPoolAllocatedSetPtr Request(KVulkanDescriptorTicket* ticket, bool& newSet);
	KVulkanGraphicsDescriptorWriter& GetGraphicsWriter(KVulkanDescriptorTicket* ticket, uint32_t threadIndex);
	std::vector<VkDescriptorSet> GetDescriptorSets(const KVulkanDescriptorTicket* ticket) const;
	void Destroy(KVulkanDescriptorTicket* ticket);
	void Release(uint32_t frameNum);
	void SetDebugName(KVulkanDescriptorTicket* ticket, const char* name);
	void UnInit();
};
