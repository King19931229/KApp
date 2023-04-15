#pragma once
#include "KVulkanConfig.h"
#include "Interface/IKRenderCommand.h"
#include <vector>
#include <mutex>

class KVulkanDescriptorPool
{
protected:
	struct DescriptorSetBlock
	{
		VkDescriptorPool pool;
		std::vector<VkDescriptorSet> sets;
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

	std::vector<VkDescriptorImageInfo> m_DynamicImageWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_UniformBufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_DynamicUniformBufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_DynamicStorageBufferWriteInfo;

	std::vector<VkWriteDescriptorSet> m_DescriptorWriteInfo;
	// 持久化的临时容器
	std::vector<VkWriteDescriptorSet> m_DescriptorDynamicWriteInfo;

	size_t m_CurrentFrame;
	size_t m_BlockSize;
	uint32_t m_UniformBufferCount;
	uint32_t m_DynamicUniformBufferCount;
	uint32_t m_DynamicStorageBufferCount;
	uint32_t m_ImageCount;

	std::mutex m_Lock;
	std::string m_Name;

	VkDescriptorSet AllocDescriptorSet(VkDescriptorPool pool);
	VkDescriptorPool CreateDescriptorPool(size_t maxCount);

	VkDescriptorSet InternalAlloc(size_t frameIndex, size_t currentFrame);

	void Move(KVulkanDescriptorPool&& rhs);
public:
	KVulkanDescriptorPool();
	~KVulkanDescriptorPool();

	KVulkanDescriptorPool(KVulkanDescriptorPool&& rhs);
	KVulkanDescriptorPool& operator=(KVulkanDescriptorPool&& rhs);

	bool Init(VkDescriptorSetLayout layout,
		const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBinding,
		const std::vector<VkWriteDescriptorSet>& writeInfo);
	bool UnInit();

	inline void SetDebugName(const std::string& name) { m_Name = name; }

	VkDescriptorSet Alloc(size_t frameIndex, size_t currentFrame, IKPipeline* pipeline,
		const KDynamicConstantBufferUsage** ppConstantUsage, size_t dynamicBufferUsageCount,
		const KStorageBufferUsage** ppStorageUsage, size_t storageBufferUsageCount);

	// TODO 加上Compute Pipeline支持
};