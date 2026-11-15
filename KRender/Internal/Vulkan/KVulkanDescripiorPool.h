#pragma once
#include "KVulkanConfig.h"
#include "Interface/IKRenderCommand.h"
#include <vector>

struct KVulkanDescriptorPoolAllocatedSet
{
	VkDescriptorSet set;
	size_t hash0;
	size_t hash1;

	KVulkanDescriptorPoolAllocatedSet()
		: set(VK_NULL_HANDEL)
		, hash0(0)
		, hash1(0)
	{}

	KVulkanDescriptorPoolAllocatedSet(VkDescriptorSet inSet)
		: set(inSet)
		, hash0(0)
		, hash1(0)
	{}
};

typedef std::shared_ptr<KVulkanDescriptorPoolAllocatedSet> KVulkanDescriptorPoolAllocatedSetPtr;

class KVulkanDescriptorPool
{
protected:
	struct DescriptorSetBlock
	{
		VkDescriptorPool pool;
		std::vector<KVulkanDescriptorPoolAllocatedSetPtr> sets;
		uint32_t useCount;
		uint32_t maxCount;

		DescriptorSetBlock()
		{
			useCount = 0;
			maxCount = 512;
			pool = VK_NULL_HANDLE;
		}
	};

	struct DescriptorSetBlockList
	{
		std::vector<DescriptorSetBlock> blocks;
		uint32_t currentFrame;
		bool occupied;

		DescriptorSetBlockList()
		{
			currentFrame = 0;
			occupied = false;
		}
	};

	VkDescriptorSetLayout m_Layout;
	std::vector<DescriptorSetBlockList> m_Descriptors;
	std::vector<VkDescriptorPoolSize> m_PoolSizes;
	bool m_Trim;

	uint32_t m_CurrentFrame;
	uint32_t m_BlockSize;

#ifdef _DEBUG
	std::atomic_bool m_Allocating;
#endif	
	std::string m_Name;

	VkDescriptorSet AllocDescriptorSet(VkDescriptorPool pool);
	VkDescriptorPool CreateDescriptorPool(uint32_t maxCount);

	KVulkanDescriptorPoolAllocatedSetPtr InternalAlloc(bool reuseSet, bool& newSet);

	void Move(KVulkanDescriptorPool&& rhs);
public:
	KVulkanDescriptorPool();
	~KVulkanDescriptorPool();

	KVulkanDescriptorPool(KVulkanDescriptorPool&) = delete;
	KVulkanDescriptorPool& operator=(KVulkanDescriptorPool&) = delete;

	KVulkanDescriptorPool(KVulkanDescriptorPool&& rhs);
	KVulkanDescriptorPool& operator=(KVulkanDescriptorPool&& rhs);

	bool Init(VkDescriptorSetLayout layout,
		const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t blockSize, bool trim);
	bool UnInit();

	void Release(uint32_t frameNum);
	bool IsOccupied() const;
	inline bool IsInitialized() const { return m_Layout != VK_NULL_HANDLE; }
	void AppendDescriptorSets(std::vector<VkDescriptorSet>& sets) const;

	KVulkanDescriptorPoolAllocatedSetPtr Alloc(bool& newSet);
	KVulkanDescriptorPoolAllocatedSetPtr Request(bool& newSet);

	inline void SetDebugName(const std::string& name) { m_Name = name; }
	inline const std::string& GetDebugName() { return m_Name; }
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
