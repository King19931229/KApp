#include "KVulkanDescripiorPool.h"
#include "KVulkanBuffer.h"
#include "KVulkanGlobal.h"
#include "KBase/Publish/KConfig.h"
#include <algorithm>
#include <assert.h>

KVulkanDescriptorPool::KVulkanDescriptorPool()
	: m_Layout(VK_NULL_HANDLE),
	m_CurrentFrame(0),
	m_BlockSize(512),
	m_UniformBufferCount(0),
	m_DyanmicUniformBufferCount(0),
	m_SamplerCount(0)
{
}

KVulkanDescriptorPool::~KVulkanDescriptorPool()
{
	ASSERT_RESULT(m_Descriptors.empty());
}

bool KVulkanDescriptorPool::Init(VkDescriptorSetLayout layout,
	const std::vector<VkDescriptorSetLayoutBinding>& m_DescriptorSetLayoutBinding,
	const std::vector<VkWriteDescriptorSet>& writeInfo)
{
	UnInit();

	m_Layout = layout;
	m_SamplerCount = 0;
	m_DyanmicUniformBufferCount = 0;
	m_UniformBufferCount = 0;

	for (const VkDescriptorSetLayoutBinding& layoutBinding : m_DescriptorSetLayoutBinding)
	{
		if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			m_SamplerCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			m_UniformBufferCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
			m_DyanmicUniformBufferCount += layoutBinding.descriptorCount;
		}
		else
		{
			ASSERT_RESULT(false && "not support now");
		}
	}

	m_ImageWriteInfo.clear();
	m_BufferWriteInfo.clear();
	m_DescriptorWriteInfo.clear();
	m_DescriptorWriteInfo.reserve(writeInfo.size());

	size_t numSampler = 0;
	size_t numUniformBuffer = 0;

	for (const VkWriteDescriptorSet& writeDescriptorSet : writeInfo)
	{
		if (writeDescriptorSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			numSampler += writeDescriptorSet.descriptorCount;
		}
		else if (writeDescriptorSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			numUniformBuffer += writeDescriptorSet.descriptorCount;
		}
		else if (writeDescriptorSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
		}
		else
		{
			ASSERT_RESULT(false && "not support now");
		}
	}

	m_ImageWriteInfo.resize(numSampler);
	m_BufferWriteInfo.resize(numUniformBuffer);
	m_DynamicBufferWriteInfo.resize(m_DyanmicUniformBufferCount);

	size_t samplerIdx = 0;
	size_t uniformBufferIdx = 0;

	for (const VkWriteDescriptorSet& writeDescriptorSet : writeInfo)
	{
		VkWriteDescriptorSet copy = writeDescriptorSet;

		copy.sType = writeDescriptorSet.sType;
		copy.pNext = writeDescriptorSet.pNext;
		copy.dstSet = VK_NULL_HANDLE;
		copy.dstBinding = writeDescriptorSet.dstBinding;
		copy.dstArrayElement = writeDescriptorSet.dstArrayElement;
		copy.descriptorCount = writeDescriptorSet.descriptorCount;
		copy.descriptorType = writeDescriptorSet.descriptorType;

		if (copy.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			size_t writeOffset = samplerIdx * sizeof(m_ImageWriteInfo[0]);
			size_t writeSize = copy.descriptorCount * sizeof(m_ImageWriteInfo[0]);
			memcpy(POINTER_OFFSET(m_ImageWriteInfo.data(), writeOffset), copy.pImageInfo, writeSize);
			copy.pImageInfo = static_cast<const VkDescriptorImageInfo*>(POINTER_OFFSET(m_ImageWriteInfo.data(), writeOffset));
			samplerIdx += copy.descriptorCount;
		}
		else if (copy.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			size_t writeOffset = uniformBufferIdx * sizeof(m_BufferWriteInfo[0]);
			size_t writeSize = copy.descriptorCount * sizeof(m_BufferWriteInfo[0]);
			memcpy(POINTER_OFFSET(m_BufferWriteInfo.data(), writeOffset), copy.pBufferInfo, writeSize);
			copy.pBufferInfo = static_cast<const VkDescriptorBufferInfo*>(POINTER_OFFSET(m_BufferWriteInfo.data(), writeOffset));
			uniformBufferIdx += copy.descriptorCount;
		}

		m_DescriptorWriteInfo.push_back(copy);
	}

	m_DescriptorDynamicWriteInfo.resize(m_DyanmicUniformBufferCount);

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
			block.sets.clear();
		}
		blockList.blocks.clear();
	}
	m_Descriptors.clear();

	m_Layout = VK_NULL_HANDLE;
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

	std::vector<VkWriteDescriptorSet> writeInfo = m_DescriptorWriteInfo;
	for (VkWriteDescriptorSet& info : writeInfo) { info.dstSet = newSet; }
	vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(writeInfo.size()), writeInfo.data(), 0, nullptr);
	return newSet;
}

VkDescriptorPool KVulkanDescriptorPool::CreateDescriptorPool(size_t maxCount)
{
	VkDescriptorPoolSize uniformPoolSize = {};
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformPoolSize.descriptorCount = (uint32_t)maxCount * m_UniformBufferCount;

	VkDescriptorPoolSize dynamicUniformPoolSize = {};
	dynamicUniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	dynamicUniformPoolSize.descriptorCount = (uint32_t)maxCount * m_DyanmicUniformBufferCount;

	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = (uint32_t)maxCount * m_SamplerCount;

	VkDescriptorPoolSize poolSizes[3];
	uint32_t poolSizeCount = 0;

	if (uniformPoolSize.descriptorCount > 0)
	{
		poolSizes[poolSizeCount++] = uniformPoolSize;
	}
	if (dynamicUniformPoolSize.descriptorCount > 0)
	{
		poolSizes[poolSizeCount++] = dynamicUniformPoolSize;
	}
	if (samplerPoolSize.descriptorCount > 0)
	{
		poolSizes[poolSizeCount++] = samplerPoolSize;
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizeCount;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = (uint32_t)maxCount;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VkDescriptorPool newPool = VK_NULL_HANDLE;
	VK_ASSERT_RESULT(vkCreateDescriptorPool(KVulkanGlobal::device, &poolInfo, nullptr, &newPool));
	return newPool;
}

VkDescriptorSet KVulkanDescriptorPool::Alloc(size_t frameIndex, size_t currentFrame, const KDynamicConstantBufferUsage** ppUsage, size_t count)
{
	VkDescriptorSet set = InternalAlloc(frameIndex, currentFrame);

	ASSERT_RESULT(set != VK_NULL_HANDLE);

	ASSERT_RESULT(!count || ppUsage);
	ASSERT_RESULT(count <= m_DynamicBufferWriteInfo.size());
	ASSERT_RESULT(count <= m_DescriptorDynamicWriteInfo.size());

	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

	for (size_t i = 0; i < count; ++i)
	{
		const KDynamicConstantBufferUsage* usage = ppUsage[i];

		IKUniformBufferPtr uniformBuffer = usage->buffer;

		VkDescriptorBufferInfo& bufferInfo = m_DynamicBufferWriteInfo[i];
		bufferInfo.buffer = ((KVulkanUniformBuffer*)uniformBuffer.get())->GetVulkanHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = usage->range;

		VkWriteDescriptorSet& dynamicUniformDescriptorWrite = m_DescriptorDynamicWriteInfo[i];

		dynamicUniformDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		dynamicUniformDescriptorWrite.dstSet = set;
		dynamicUniformDescriptorWrite.dstBinding = (uint32_t)usage->binding;
		dynamicUniformDescriptorWrite.dstArrayElement = 0;
		dynamicUniformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		dynamicUniformDescriptorWrite.descriptorCount = 1;

		dynamicUniformDescriptorWrite.pBufferInfo = &bufferInfo;
		dynamicUniformDescriptorWrite.pImageInfo = nullptr;
		dynamicUniformDescriptorWrite.pTexelBufferView = nullptr;
	}

	vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(count), m_DescriptorDynamicWriteInfo.data(), 0, nullptr);

	return set;
}

VkDescriptorSet KVulkanDescriptorPool::InternalAlloc(size_t frameIndex, size_t currentFrame)
{
	ASSERT_RESULT(m_Layout != VK_NULL_HANDLE);

	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

	if (frameIndex >= m_Descriptors.size())
	{
		m_Descriptors.resize(frameIndex + 1);
	}

	DescriptorSetBlockList& blockList = m_Descriptors[frameIndex];

	if (blockList.currentFrame != currentFrame)
	{
		blockList.currentFrame = currentFrame;

		for (auto it = blockList.blocks.begin(); it != blockList.blocks.end();)
		{
			DescriptorSetBlock& block = *it;

			if (block.useCount < block.sets.size() / 2)
			{
				size_t newCount = block.sets.size() / 2;
				for (size_t idx = newCount; idx < block.sets.size(); ++idx)
				{
					vkFreeDescriptorSets(KVulkanGlobal::device, block.pool, 1, &block.sets[idx]);
				}
				block.sets.resize(newCount);

				if (block.sets.size() == 0 && blockList.blocks.size() > 1)
				{
					vkDestroyDescriptorPool(KVulkanGlobal::device, block.pool, nullptr);
					it = blockList.blocks.erase(it);
				}
				else
				{
					++it;
				}
			}
			else
			{
				++it;
			}
		}

		for (auto it = blockList.blocks.begin(), itEnd = blockList.blocks.end(); it != itEnd; ++it)
		{
			it->useCount = 0;
		}
	}

	for (auto it = blockList.blocks.begin(), itEnd = blockList.blocks.end(); it != itEnd; ++it)
	{
		DescriptorSetBlock& block = *it;
		size_t newUseIndex = block.useCount++;
		if (newUseIndex < block.maxCount)
		{
			if (newUseIndex >= block.sets.size())
			{
				VkDescriptorSet newSet = AllocDescriptorSet(block.pool);
				block.sets.push_back(newSet);
				return newSet;
			}
			else
			{
				return block.sets[newUseIndex];
			}
		}
	}

	DescriptorSetBlock newBlock;
	newBlock.useCount = 0;
	newBlock.maxCount = m_BlockSize;
	newBlock.pool = CreateDescriptorPool(newBlock.maxCount);

	VkDescriptorSet newSet = AllocDescriptorSet(newBlock.pool);
	newBlock.sets.push_back(newSet);
	++newBlock.useCount;

	blockList.blocks.push_back(newBlock);
	return newSet;
}