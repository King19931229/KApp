#include "KVulkanDescripiorPool.h"
#include "KVulkanGlobal.h"
#include "KBase/Publish/KConfig.h"
#include <algorithm>
#include <assert.h>

KVulkanDescriptorPool::KVulkanDescriptorPool()
	: m_Layout(VK_NULL_HANDLE),
	m_CurrentFrame(0),
	m_UniformBufferCount(0),
	m_SamplerCount(0)
{
}

KVulkanDescriptorPool::~KVulkanDescriptorPool()
{
	ASSERT_RESULT(m_Descriptors.empty());
}

bool KVulkanDescriptorPool::Init(VkDescriptorSetLayout layout, const std::vector<VkWriteDescriptorSet>& writeInfo, uint32_t uniformBufferCount, uint32_t samplerCount)
{
	UnInit();
	ASSERT_RESULT(layout != VK_NULL_HANDLE);
	m_Layout = layout;
	m_WriteInfo = writeInfo;
	m_UniformBufferCount = uniformBufferCount;
	m_SamplerCount = samplerCount;
	return true;
}

bool KVulkanDescriptorPool::UnInit()
{
	for (DescriptorSetBlockList& blockList : m_Descriptors)
	{
		for (DescriptorSetBlock& block : blockList.blocks)
		{
			vkDestroyDescriptorPool(KVulkanGlobal::device, block.pool, nullptr);
			block.pool = VK_NULL_HANDLE;
			block.set = VK_NULL_HANDLE;
		}
		blockList.blocks.clear();
	}
	m_Descriptors.clear();

	m_Layout = VK_NULL_HANDLE;
	m_UniformBufferCount = 0;
	m_SamplerCount = 0;
	return true;
}

VkDescriptorSet KVulkanDescriptorPool::AllocDescriptorSet(VkDescriptorPool pool)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_Layout;

	VkDescriptorSet newSet = VK_NULL_HANDLE;
	VK_ASSERT_RESULT(vkAllocateDescriptorSets(KVulkanGlobal::device, &allocInfo, &newSet));

	std::vector<VkWriteDescriptorSet> writeInfo = m_WriteInfo;
	for (VkWriteDescriptorSet& info : writeInfo) { info.dstSet = newSet; }
	vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(writeInfo.size()), writeInfo.data(), 0, nullptr);
	return newSet;
}

VkDescriptorPool KVulkanDescriptorPool::CreateDescriptorPool()
{
	VkDescriptorPoolSize uniformPoolSize = {};
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformPoolSize.descriptorCount = std::max(1U, m_UniformBufferCount);

	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = std::max(1U, m_SamplerCount);

	VkDescriptorPoolSize poolSizes[] = { uniformPoolSize, samplerPoolSize };

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = ARRAY_SIZE(poolSizes);
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 1;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VkDescriptorPool newPool = VK_NULL_HANDLE;
	VK_ASSERT_RESULT(vkCreateDescriptorPool(KVulkanGlobal::device, &poolInfo, nullptr, &newPool));
	return newPool;
}

VkDescriptorSet KVulkanDescriptorPool::Alloc(size_t frameIndex, size_t currentFrame)
{
	ASSERT_RESULT(m_Layout != VK_NULL_HANDLE);

	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

		if (frameIndex >= m_Descriptors.size())
		{
			m_Descriptors.resize(frameIndex + 1);
		}

		DescriptorSetBlockList& blockList = m_Descriptors[frameIndex];

		if (blockList.currentFrame != currentFrame)
		{
			blockList.currentFrame = currentFrame;
			if (blockList.useCount < blockList.blocks.size() / 2)
			{
				size_t clearIdx = 0;
				size_t newCount = blockList.blocks.size() / 2;

				if (blockList.useCount == 0 || newCount == 0)
				{
					clearIdx = 0;
					newCount = 0;
				}
				else
				{
					clearIdx = newCount;
				}

				for (size_t idx = clearIdx; idx < blockList.blocks.size(); ++idx)
				{
					DescriptorSetBlock& block = blockList.blocks[idx];
					vkDestroyDescriptorPool(KVulkanGlobal::device, block.pool, nullptr);
				}
				blockList.blocks.resize(newCount);
			}
			blockList.useCount = 0;
		}

		if (blockList.useCount < blockList.blocks.size())
		{
			return blockList.blocks[blockList.useCount++].set;
		}
	}

	DescriptorSetBlock newBlock;
	newBlock.pool = CreateDescriptorPool();
	newBlock.set = AllocDescriptorSet(newBlock.pool);

	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		DescriptorSetBlockList& blockList = m_Descriptors[frameIndex];
		blockList.blocks.push_back(newBlock);
		++blockList.useCount;
	}

	return newBlock.set;
}