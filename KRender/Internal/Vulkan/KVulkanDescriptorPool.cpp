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
	m_StaticUniformBufferCount(0),
	m_DynamicUniformBufferCount(0),
	m_DynamicStorageBufferCount(0),
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
	m_DynamicUniformBufferCount = 0;
	m_StaticUniformBufferCount = 0;
	m_DynamicStorageBufferCount = 0;

	for (const VkDescriptorSetLayoutBinding& layoutBinding : descriptorSetLayoutBinding)
	{
		if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			m_SamplerCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			m_StaticUniformBufferCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
			m_DynamicUniformBufferCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			m_DynamicStorageBufferCount += layoutBinding.descriptorCount;
		}
		else
		{
			ASSERT_RESULT(false && "not support now");
		}
	}

	m_DynamicImageWriteInfo.clear();
	m_StaticUniformBufferWriteInfo.clear();
	m_DescriptorWriteInfo.clear();
	m_DescriptorWriteInfo.reserve(writeInfo.size());

	m_DynamicImageWriteInfo.resize(m_SamplerCount);
	m_StaticUniformBufferWriteInfo.resize(m_StaticUniformBufferCount);
	m_DynamicUniformBufferWriteInfo.resize(m_DynamicUniformBufferCount);
	m_DynamicStorageBufferWriteInfo.resize(m_DynamicStorageBufferCount);

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
			size_t writeOffset = uniformBufferIdx * sizeof(m_StaticUniformBufferWriteInfo[0]);
			size_t writeSize = copy.descriptorCount * sizeof(m_StaticUniformBufferWriteInfo[0]);
			memcpy(POINTER_OFFSET(m_StaticUniformBufferWriteInfo.data(), writeOffset), copy.pBufferInfo, writeSize);
			copy.pBufferInfo = static_cast<const VkDescriptorBufferInfo*>(POINTER_OFFSET(m_StaticUniformBufferWriteInfo.data(), writeOffset));
			uniformBufferIdx += copy.descriptorCount;
		}

		m_DescriptorWriteInfo.push_back(copy);
	}

	// 取最大值
	m_DescriptorDynamicWriteInfo.resize(std::max(std::max(m_DynamicUniformBufferCount, m_SamplerCount), m_DynamicStorageBufferCount));

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
	VkDescriptorPoolSize storagePoolSize = {};
	storagePoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	storagePoolSize.descriptorCount = (uint32_t)maxCount * m_DynamicStorageBufferCount;

	VkDescriptorPoolSize uniformPoolSize = {};
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformPoolSize.descriptorCount = (uint32_t)maxCount * m_StaticUniformBufferCount;

	VkDescriptorPoolSize dynamicUniformPoolSize = {};
	dynamicUniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	dynamicUniformPoolSize.descriptorCount = (uint32_t)maxCount * m_DynamicUniformBufferCount;

	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = (uint32_t)maxCount * m_SamplerCount;

	VkDescriptorPoolSize poolSizes[4];
	uint32_t poolSizeCount = 0;

	if (storagePoolSize.descriptorCount > 0)
	{
		poolSizes[poolSizeCount++] = storagePoolSize;
	}
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

VkDescriptorSet KVulkanDescriptorPool::Alloc(size_t frameIndex, size_t currentFrame, IKPipeline* pipeline,
	const KDynamicConstantBufferUsage** ppConstantUsage, size_t dynamicBufferUsageCount,
	const KStorageBufferUsage** ppStorageUsage, size_t storageBufferUsageCount)
{
	VkDescriptorSet set = InternalAlloc(frameIndex, currentFrame);

	ASSERT_RESULT(set != VK_NULL_HANDLE);

	ASSERT_RESULT(!dynamicBufferUsageCount || ppConstantUsage);
	ASSERT_RESULT(dynamicBufferUsageCount <= m_DynamicUniformBufferWriteInfo.size());
	ASSERT_RESULT(dynamicBufferUsageCount <= m_DescriptorDynamicWriteInfo.size());

	std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);

	KVulkanPipeline* vulkanPipeline = static_cast<KVulkanPipeline*>(pipeline);
	if (vulkanPipeline)
	{
		std::unordered_map<unsigned int, KVulkanPipeline::SamplerBindingInfo>& samplers = vulkanPipeline->m_Samplers;
		ASSERT_RESULT(samplers.size() <= m_DynamicImageWriteInfo.size());
		ASSERT_RESULT(samplers.size() <= m_DescriptorDynamicWriteInfo.size());

		size_t idx = 0;
		for (auto& pair : samplers)
		{
			unsigned int binding = pair.first;
			KVulkanPipeline::SamplerBindingInfo& info = pair.second;

			if (!info.dynamicWrite && info.onceWrite)
			{
				continue;
			}

			VkDescriptorImageInfo &imageInfo = m_DynamicImageWriteInfo[idx];

			IKFrameBufferPtr frameBuffer = info.image;
			ASSERT_RESULT(frameBuffer);

			imageInfo.imageLayout = frameBuffer->IsStroageImage() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageLayout = frameBuffer->IsDepthStencil() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : imageInfo.imageLayout;
			imageInfo.imageView = ((KVulkanFrameBuffer*)frameBuffer.get())->GetImageView();
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
		const KDynamicConstantBufferUsage* usage = ppConstantUsage[i];

		IKUniformBufferPtr uniformBuffer = usage->buffer;

		VkDescriptorBufferInfo& bufferInfo = m_DynamicUniformBufferWriteInfo[i];
		bufferInfo.buffer = ((KVulkanUniformBuffer*)uniformBuffer.get())->GetVulkanHandle();
		// offset 不在这里绑定 在KVulkanCommandBuffer::Render时候绑定
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

	for (size_t i = 0; i < storageBufferUsageCount; ++i)
	{
		const KStorageBufferUsage* usage = ppStorageUsage[i];

		IKVertexBufferPtr buffer = usage->buffer;

		VkDescriptorBufferInfo& bufferInfo = m_DynamicStorageBufferWriteInfo[i];
		bufferInfo.buffer = ((KVulkanVertexBuffer*)buffer.get())->GetVulkanHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet& storageDescriptorWrite = m_DescriptorDynamicWriteInfo[i];

		storageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		storageDescriptorWrite.dstSet = set;
		storageDescriptorWrite.dstBinding = (uint32_t)usage->binding;
		storageDescriptorWrite.dstArrayElement = 0;
		storageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storageDescriptorWrite.descriptorCount = 1;

		storageDescriptorWrite.pBufferInfo = &bufferInfo;
		storageDescriptorWrite.pImageInfo = nullptr;
		storageDescriptorWrite.pTexelBufferView = nullptr;
	}

	vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(storageBufferUsageCount), m_DescriptorDynamicWriteInfo.data(), 0, nullptr);

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