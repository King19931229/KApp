#pragma once
#include "KVulkanConfig.h"
#include "Interface/IKRenderCommand.h"
#include <vector>
#include <mutex>

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
		size_t useCount;
		size_t maxCount;

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
		size_t currentFrame;

		DescriptorSetBlockList()
		{
			currentFrame = 0;
		}
	};

	VkDescriptorSetLayout m_Layout;
	std::vector<DescriptorSetBlockList> m_Descriptors;

	std::vector<VkDescriptorImageInfo>  m_ImageWriteInfo;
	std::vector<VkDescriptorImageInfo>  m_StorageImageWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_StorageBufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_UniformBufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_DynamicUniformBufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_DynamicStorageBufferWriteInfo;

	std::vector<VkWriteDescriptorSet> m_DescriptorStaticWriteInfo;
	std::vector<VkWriteDescriptorSet> m_DescriptorDynamicWriteInfo;

	size_t m_CurrentFrame;
	size_t m_BlockSize;

	uint32_t m_ImageCount;
	uint32_t m_StorageImageCount;
	uint32_t m_UniformBufferCount;
	uint32_t m_StorageBufferCount;
	uint32_t m_DynamicUniformBufferCount;
	uint32_t m_DynamicStorageBufferCount;

#ifdef _DEBUG
	std::atomic_bool m_Allocating;
#endif	
	std::string m_Name;

	VkDescriptorSet AllocDescriptorSet(VkDescriptorPool pool);
	VkDescriptorPool CreateDescriptorPool(size_t maxCount);

	size_t HashCombine(const VkWriteDescriptorSet& write, size_t currentHash);
	KVulkanDescriptorPoolAllocatedSetPtr InternalAlloc(size_t frameIndex, size_t currentFrame);

	void Move(KVulkanDescriptorPool&& rhs);
public:
	KVulkanDescriptorPool();
	~KVulkanDescriptorPool();

	KVulkanDescriptorPool(KVulkanDescriptorPool&) = delete;
	KVulkanDescriptorPool& operator=(KVulkanDescriptorPool&) = delete;

	KVulkanDescriptorPool(KVulkanDescriptorPool&& rhs);
	KVulkanDescriptorPool& operator=(KVulkanDescriptorPool&& rhs);

	bool Init(VkDescriptorSetLayout layout,
		const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBinding,
		const std::vector<VkWriteDescriptorSet>& writeInfo);
	bool UnInit();

	bool Trim(size_t frameIndex, size_t currentFrame);

	inline void SetDebugName(const std::string& name) { m_Name = name; }

	VkDescriptorSet Alloc(size_t frameIndex, size_t currentFrame, IKPipeline* pipeline,
		const KDynamicConstantBufferUsage** ppConstantUsage, size_t dynamicBufferUsageCount,
		const KStorageBufferUsage** ppStorageUsage, size_t storageBufferUsageCount);
};