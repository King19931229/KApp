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

	std::vector<VkDescriptorImageInfo> m_ImageWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_BufferWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_DynamicBufferWriteInfo;

	std::vector<VkWriteDescriptorSet> m_DescriptorWriteInfo;
	std::vector<VkWriteDescriptorSet> m_DescriptorDynamicWriteInfo;

	size_t m_CurrentFrame;
	size_t m_BlockSize;
	uint32_t m_UniformBufferCount;
	uint32_t m_DyanmicUniformBufferCount;
	uint32_t m_SamplerCount;

	std::mutex m_Lock;

	VkDescriptorSet AllocDescriptorSet(VkDescriptorPool pool);
	VkDescriptorPool CreateDescriptorPool(size_t maxCount);

	VkDescriptorSet InternalAlloc(size_t frameIndex, size_t currentFrame);
public:
	KVulkanDescriptorPool();
	~KVulkanDescriptorPool();

	bool Init(VkDescriptorSetLayout layout,
		const std::vector<VkDescriptorSetLayoutBinding>& m_DescriptorSetLayoutBinding,
		const std::vector<VkWriteDescriptorSet>& writeInfo);
	bool UnInit();

	VkDescriptorSet Alloc(size_t frameIndex, size_t currentFrame, const KDynamicConstantBufferUsage** ppUsage, size_t count);
};