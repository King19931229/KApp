#pragma once
#include "KVulkanDescripiorPool.h"
#include <list>
#include <map>
#include <memory>
#include <mutex>

struct KVulkanDescriptorTicket
{
private:
	friend class KVulkanDescriptorManager;
	struct ThreadResources
	{
		KVulkanDescriptorPool pool;
		std::unique_ptr<KVulkanGraphicsDescriptorWriter> writer;
	};

	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> poolSizes;
	std::vector<std::unique_ptr<ThreadResources>> threads;
	std::vector<uint32_t> activeThreads;
	uint32_t blockSize = 0;
	bool trim = false;
	bool retired = false;
	std::string name;

public:
	explicit KVulkanDescriptorTicket(uint32_t threadCount) : threads(threadCount) {}
};

class KVulkanDescriptorManager
{
protected:
	struct CommandPoolResources
	{
		uint32_t threadIndex;
		std::multimap<VkDescriptorSetLayout, std::weak_ptr<KVulkanDescriptorArena>> arenas;
	};

	std::list<KVulkanDescriptorTicket> m_Tickets;
	std::map<VkCommandPool, CommandPoolResources> m_CommandPools;
	mutable std::mutex m_Lock;

	KVulkanDescriptorTicket::ThreadResources& GetThreadResources(KVulkanDescriptorTicket* ticket, uint32_t threadIndex);
	std::shared_ptr<KVulkanDescriptorArena> GetArena(KVulkanDescriptorTicket* ticket, uint32_t threadIndex, VkCommandPool commandPool);
	void DestroyUnusedTickets();
public:
	~KVulkanDescriptorManager();
	KVulkanDescriptorTicket* Create(VkDescriptorSetLayout layout,
		const std::vector<VkDescriptorSetLayoutBinding>& bindings,
		uint32_t threadCount, uint32_t blockSize, bool trim);
	KVulkanDescriptorPoolAllocatedSetPtr Alloc(KVulkanDescriptorTicket* ticket, uint32_t threadIndex, bool& newSet, VkCommandPool commandPool = VK_NULL_HANDLE);
	KVulkanDescriptorPoolAllocatedSetPtr Request(KVulkanDescriptorTicket* ticket, bool& newSet);
	KVulkanGraphicsDescriptorWriter& GetGraphicsWriter(KVulkanDescriptorTicket* ticket, uint32_t threadIndex);
	std::vector<VkDescriptorSet> GetDescriptorSets(const KVulkanDescriptorTicket* ticket) const;
	void Destroy(KVulkanDescriptorTicket* ticket);
	void Release(uint32_t frameNum);
	void ReleaseCommandPool(VkCommandPool commandPool);
	void SetDebugName(KVulkanDescriptorTicket* ticket, const char* name);
	void UnInit();
};
