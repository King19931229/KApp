#include "KVulkanDescripiorPool.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanPipeline.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanGlobal.h"
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
	const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBinding,
	const std::vector<VkWriteDescriptorSet>& writeInfo)
{
	UnInit();

	m_Layout = layout;
	m_SamplerCount = 0;
	m_DyanmicUniformBufferCount = 0;
	m_UniformBufferCount = 0;

	for (const VkDescriptorSetLayoutBinding& layoutBinding : descriptorSetLayoutBinding)
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
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{

		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{

		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
		{

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
		if (copy.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			size_t writeOffset = uniformBufferIdx * sizeof(m_BufferWriteInfo[0]);
			size_t writeSize = copy.descriptorCount * sizeof(m_BufferWriteInfo[0]);
			memcpy(POINTER_OFFSET(m_BufferWriteInfo.data(), writeOffset), copy.pBufferInfo, writeSize);
			copy.pBufferInfo = static_cast<const VkDescriptorBufferInfo*>(POINTER_OFFSET(m_BufferWriteInfo.data(), writeOffset));
			uniformBufferIdx += copy.descriptorCount;
		}
		m_DescriptorWriteInfo.push_back(copy);
	}

	// 取Image与DynamicBuffer最大值
	m_DescriptorDynamicWriteInfo.resize(std::max(m_DyanmicUniformBufferCount, m_SamplerCount));

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

VkDescriptorSet KVulkanDescriptorPool::Alloc(size_t frameIndex, size_t currentFrame, IKPipeline* pipeline, const KDynamicConstantBufferUsage** ppBufferUsage, size_t dynamicBufferUsageCount)
{
	VkDescriptorSet set = InternalAlloc(frameIndex, currentFrame);

	ASSERT_RESULT(set != VK_NULL_HANDLE);

	ASSERT_RESULT(!dynamicBufferUsageCount || ppBufferUsage);
	ASSERT_RESULT(dynamicBufferUsageCount <= m_DynamicBufferWriteInfo.size());
	ASSERT_RESULT(dynamicBufferUsageCount <= m_DescriptorDynamicWriteInfo.size());

	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

	KVulkanPipeline* vulkanPipeline = static_cast<KVulkanPipeline*>(pipeline);
	if (vulkanPipeline)
	{
		std::unordered_map<unsigned int, KVulkanPipeline::SamplerBindingInfo>& samplers = vulkanPipeline->m_Samplers;
		ASSERT_RESULT(samplers.size() <= m_ImageWriteInfo.size());
		ASSERT_RESULT(samplers.size() <= m_DescriptorDynamicWriteInfo.size());

		size_t idx = 0;
		for (auto& pair : samplers)
		{
			unsigned int binding = pair.first;
			KVulkanPipeline::SamplerBindingInfo& info = pair.second;

			if (!info.dynamicWrite & info.onceWrite)
			{
				continue;
			}

			VkDescriptorImageInfo &imageInfo = m_ImageWriteInfo[idx];

			if (!info.frameBuffer && info.texture)
			{
				info.frameBuffer = info.texture->GetFrameBuffer();
			}

			imageInfo.imageLayout = info.frameBuffer->IsDepthStencil() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = ((KVulkanFrameBuffer*)info.frameBuffer.get())->GetImageView();
			imageInfo.sampler = ((KVulkanSampler*)info.sampler.get())->GetVkSampler();

			ASSERT_RESULT(imageInfo.imageView);
			ASSERT_RESULT(imageInfo.sampler);

			VkWriteDescriptorSet& samplerDescriptorWrite = m_DescriptorDynamicWriteInfo[idx];

			samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			samplerDescriptorWrite.dstSet = set;
			samplerDescriptorWrite.dstBinding = (uint32_t)binding;
			samplerDescriptorWrite.dstArrayElement = 0;
			samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerDescriptorWrite.descriptorCount = 1;

			samplerDescriptorWrite.pBufferInfo = nullptr;
			samplerDescriptorWrite.pImageInfo = &imageInfo;
			samplerDescriptorWrite.pTexelBufferView = nullptr;

			info.onceWrite = true;

			++idx;
		}

		vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(idx), m_DescriptorDynamicWriteInfo.data(), 0, nullptr);
	}

	for (size_t i = 0; i < dynamicBufferUsageCount; ++i)
	{
		const KDynamicConstantBufferUsage* usage = ppBufferUsage[i];

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

	vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(dynamicBufferUsageCount), m_DescriptorDynamicWriteInfo.data(), 0, nullptr);

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