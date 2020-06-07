#pragma once
#include "KVulkanConfig.h"
#include <vector>
#include <mutex>

class KVulkanDescriptorPool
{
protected:
	struct DescriptorSetBlock
	{
		VkDescriptorPool pool;
		VkDescriptorSet set;

		DescriptorSetBlock()
		{
			pool = VK_NULL_HANDLE;
			set = VK_NULL_HANDLE;
		}
	};

	struct DescriptorSetBlockList
	{
		std::vector<DescriptorSetBlock> blocks;
		size_t useCount;
		size_t currentFrame;

		DescriptorSetBlockList()
		{
			useCount = 0;
			currentFrame = 0;
		}
	};

	VkDescriptorSetLayout m_Layout;
	std::vector<DescriptorSetBlockList> m_Descriptors;

	std::vector<VkDescriptorImageInfo> m_ImageWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_BufferWriteInfo;
	std::vector<VkWriteDescriptorSet> m_DescriptorWriteInfo;

	size_t m_CurrentFrame;
	uint32_t m_UniformBufferCount;
	uint32_t m_SamplerCount;
	std::mutex m_Lock;

	VkDescriptorSet AllocDescriptorSet(VkDescriptorPool pool);
	VkDescriptorPool CreateDescriptorPool();
public:
	KVulkanDescriptorPool();
	~KVulkanDescriptorPool();

	bool Init(VkDescriptorSetLayout layout,
		const std::vector<VkDescriptorSetLayoutBinding>& m_DescriptorSetLayoutBinding,
		const std::vector<VkWriteDescriptorSet>& writeInfo);
	bool UnInit();

	VkDescriptorSet Alloc(size_t frameIndex, size_t currentFrame);
};