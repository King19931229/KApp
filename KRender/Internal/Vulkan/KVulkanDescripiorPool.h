#pragma once
#include "KVulkanConfig.h"
#include "Interface/IKRenderCommand.h"
#include <vector>
#include <functional>

struct KVulkanDescriptorPoolAllocatedSet
{
	VkDescriptorSet set;
	VkCommandPool commandPool;
	size_t hash0;
	size_t hash1;

	KVulkanDescriptorPoolAllocatedSet(VkDescriptorSet inSet, VkCommandPool inCommandPool)
		: set(inSet)
		, commandPool(inCommandPool)
		, hash0(0)
		, hash1(0)
	{}
};

typedef std::shared_ptr<KVulkanDescriptorPoolAllocatedSet> KVulkanDescriptorPoolAllocatedSetPtr;

// 同一 CommandPool 内的管线共享底层容量，各自保留 DescriptorSet 缓存。
class KVulkanDescriptorArena
{
	struct Page
	{
		VkDescriptorPool pool;
		uint32_t capacity;
		uint32_t reserved;
	};
	std::vector<VkDescriptorPoolSize> m_PoolSizes;
	std::vector<Page> m_Pages;
	uint32_t m_NextPageSize = 8;
public:
	explicit KVulkanDescriptorArena(const std::vector<VkDescriptorPoolSize>& poolSizes);
	~KVulkanDescriptorArena();
	VkDescriptorPool Acquire(uint32_t count);
	void Release(VkDescriptorPool pool, uint32_t count);
	bool Matches(const std::vector<VkDescriptorPoolSize>& sizes) const;
};

class KVulkanDescriptorPool
{
protected:
	struct DescriptorSetBlock
	{
		VkDescriptorPool pool = VK_NULL_HANDLE;
		std::vector<KVulkanDescriptorPoolAllocatedSetPtr> sets;
		uint32_t useCount = 0;
		uint32_t maxCount = 0;
	};

	struct DescriptorSetBlockList
	{
		std::vector<DescriptorSetBlock> blocks;
		uint32_t currentFrame = 0;
		bool occupied = false;
		VkCommandPool commandPool = VK_NULL_HANDLE;
		std::shared_ptr<KVulkanDescriptorArena> arena;
	};

	VkDescriptorSetLayout m_Layout;
	std::vector<DescriptorSetBlockList> m_Descriptors;
	std::vector<VkDescriptorPoolSize> m_PoolSizes;
	bool m_Trim;
	std::function<std::shared_ptr<KVulkanDescriptorArena>(VkCommandPool)> m_ArenaProvider;

	uint32_t m_BlockSize;

#ifdef _DEBUG
	std::atomic_bool m_Allocating;
#endif	
	std::string m_Name;

	VkDescriptorSet AllocDescriptorSet(VkDescriptorPool pool);
	VkDescriptorPool CreateDescriptorPool(uint32_t maxCount);
	void DestroyDescriptorPool(DescriptorSetBlockList& list, DescriptorSetBlock& block);
	void Clear(DescriptorSetBlockList& list);

	KVulkanDescriptorPoolAllocatedSetPtr InternalAlloc(bool reuseSet, bool& newSet, VkCommandPool commandPool);

public:
	KVulkanDescriptorPool();
	~KVulkanDescriptorPool();

	KVulkanDescriptorPool(const KVulkanDescriptorPool&) = delete;
	KVulkanDescriptorPool& operator=(const KVulkanDescriptorPool&) = delete;

	bool Init(VkDescriptorSetLayout layout,
		const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t blockSize, bool trim);
	bool UnInit();

	void Release(uint32_t frameNum);
	void ReleaseCommandPool(VkCommandPool commandPool);
	void SetArenaProvider(std::function<std::shared_ptr<KVulkanDescriptorArena>(VkCommandPool)> provider)
	{
		m_ArenaProvider = std::move(provider);
	}
	bool IsOccupied() const;
	void AppendDescriptorSets(std::vector<VkDescriptorSet>& sets) const;

	KVulkanDescriptorPoolAllocatedSetPtr Alloc(bool& newSet, VkCommandPool commandPool = VK_NULL_HANDLE);
	KVulkanDescriptorPoolAllocatedSetPtr Request(bool& newSet);

	inline void SetDebugName(const std::string& name) { m_Name = name; }
};

class KVulkanPipeline;

class KVulkanGraphicsDescriptorWriter
{
protected:
	std::vector<VkDescriptorImageInfo>  m_ImageWriteInfo;
	std::vector<VkDescriptorImageInfo>  m_StorageImageWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_StorageBufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_UniformBufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_DynamicUniformBufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_DynamicStorageBufferWriteInfo;

	std::vector<VkWriteDescriptorSet> m_DescriptorDynamicWriteInfo;

	uint32_t m_ImageCount;
	uint32_t m_StorageImageCount;
	uint32_t m_UniformBufferCount;
	uint32_t m_StorageBufferCount;
	uint32_t m_DynamicUniformBufferCount;
	uint32_t m_DynamicStorageBufferCount;

	size_t HashCombine(const VkWriteDescriptorSet& write, size_t currentHash);
public:
	KVulkanGraphicsDescriptorWriter();
	bool Init(const std::vector<VkDescriptorPoolSize>& poolSizes);

	VkDescriptorSet UpdateDescriptorSet(KVulkanPipeline* pipeline,
		const KVulkanDescriptorPoolAllocatedSetPtr& allocation, bool newSet,
		const KDynamicConstantBufferUsage** ppConstantUsage, uint32_t dynamicBufferUsageCount,
		const KStorageBufferUsage** ppStorageUsage, uint32_t storageBufferUsageCount);
};
