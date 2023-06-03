#include "KVulkanDescripiorPool.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanPipeline.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "KBase/Publish/KHash.h"
#include "KBase/Publish/KSectionEnterAssertGuard.h"
#include <algorithm>

KVulkanDescriptorPool::KVulkanDescriptorPool()
	: m_Layout(VK_NULL_HANDLE)
	, m_CurrentFrame(0)
	, m_BlockSize(512)
	, m_ImageCount(0)
	, m_StorageImageCount(0)
	, m_UniformBufferCount(0)
	, m_StorageBufferCount(0)
	, m_DynamicUniformBufferCount(0)
	, m_DynamicStorageBufferCount(0)
#ifdef _DEBUG
	, m_Allocating(false)
#endif
{
}

KVulkanDescriptorPool::~KVulkanDescriptorPool()
{
	ASSERT_RESULT(m_Descriptors.empty());
}

KVulkanDescriptorPool::KVulkanDescriptorPool(KVulkanDescriptorPool&& rhs)
{
	Move(std::move(rhs));
}

KVulkanDescriptorPool& KVulkanDescriptorPool::operator=(KVulkanDescriptorPool&& rhs)
{
	Move(std::move(rhs));
	return *this;
}

void KVulkanDescriptorPool::Move(KVulkanDescriptorPool&& rhs)
{
	m_Layout = std::move(rhs.m_Layout);
	m_Descriptors = std::move(rhs.m_Descriptors);

	m_ImageWriteInfo = std::move(rhs.m_ImageWriteInfo);
	m_StorageBufferWriteInfo = std::move(rhs.m_StorageBufferWriteInfo);
	m_StorageImageWriteInfo = std::move(rhs.m_StorageImageWriteInfo);
	m_DynamicUniformBufferWriteInfo = std::move(rhs.m_DynamicUniformBufferWriteInfo);
	m_DynamicStorageBufferWriteInfo = std::move(rhs.m_DynamicStorageBufferWriteInfo);

	m_DescriptorStaticWriteInfo = std::move(rhs.m_DescriptorStaticWriteInfo);
	m_DescriptorDynamicWriteInfo = std::move(rhs.m_DescriptorDynamicWriteInfo);

	m_CurrentFrame = std::move(rhs.m_CurrentFrame);
	m_BlockSize = std::move(rhs.m_BlockSize);

	m_ImageCount = std::move(rhs.m_ImageCount);
	m_StorageImageCount = std::move(m_StorageImageCount);
	m_UniformBufferCount = std::move(rhs.m_UniformBufferCount);
	m_StorageBufferCount = std::move(rhs.m_StorageBufferCount);
	m_DynamicUniformBufferCount = std::move(rhs.m_DynamicUniformBufferCount);
	m_DynamicStorageBufferCount = std::move(rhs.m_DynamicStorageBufferCount);

	// m_Lock = std::move(rhs.m_Lock);
	m_Name = std::move(rhs.m_Name);
}

bool KVulkanDescriptorPool::Init(VkDescriptorSetLayout layout,
	const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBinding,
	const std::vector<VkWriteDescriptorSet>& writeInfo)
{
	UnInit();

	ASSERT_RESULT(layout);

	m_Layout = layout;
	m_ImageCount = 0;
	m_StorageImageCount = 0;
	m_UniformBufferCount = 0;
	m_StorageBufferCount = 0;
	m_DynamicUniformBufferCount = 0;
	m_DynamicStorageBufferCount = 0;

	for (const VkDescriptorSetLayoutBinding& layoutBinding : descriptorSetLayoutBinding)
	{
		if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			m_ImageCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			m_StorageImageCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			m_StorageBufferCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
			m_DynamicUniformBufferCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
		{
			m_DynamicStorageBufferCount += layoutBinding.descriptorCount;
		}
		else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			m_UniformBufferCount += layoutBinding.descriptorCount;
		}
		else
		{
			ASSERT_RESULT(false && "should not reach");
		}
	}

	m_ImageWriteInfo.resize(m_ImageCount);
	m_StorageImageWriteInfo.resize(m_StorageImageCount);
	m_StorageBufferWriteInfo.resize(m_StorageBufferCount);
	m_DynamicUniformBufferWriteInfo.resize(m_DynamicUniformBufferCount);
	m_DynamicStorageBufferWriteInfo.resize(m_DynamicStorageBufferCount);

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

		if (copy.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			ASSERT_RESULT(false && "should not reach");
		}

		m_DescriptorStaticWriteInfo.push_back(copy);
	}

	m_DescriptorDynamicWriteInfo.resize(m_ImageCount + m_StorageBufferCount + m_UniformBufferCount + m_DynamicUniformBufferCount + m_DynamicStorageBufferCount);

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

	std::vector<VkWriteDescriptorSet> writeInfo = m_DescriptorStaticWriteInfo;
	for (VkWriteDescriptorSet& info : writeInfo)
	{
		info.dstSet = newSet;
	}
	vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(writeInfo.size()), writeInfo.data(), 0, nullptr);
	return newSet;
}

VkDescriptorPool KVulkanDescriptorPool::CreateDescriptorPool(size_t maxCount)
{
	VkDescriptorPoolSize storagePoolSize = {};
	storagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storagePoolSize.descriptorCount = (uint32_t)maxCount * m_StorageBufferCount;

	VkDescriptorPoolSize dynamicStoragePoolSize = {};
	dynamicStoragePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	dynamicStoragePoolSize.descriptorCount = (uint32_t)maxCount * m_DynamicStorageBufferCount;

	VkDescriptorPoolSize uniformPoolSize = {};
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformPoolSize.descriptorCount = (uint32_t)maxCount * m_UniformBufferCount;

	VkDescriptorPoolSize dynamicUniformPoolSize = {};
	dynamicUniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	dynamicUniformPoolSize.descriptorCount = (uint32_t)maxCount * m_DynamicUniformBufferCount;

	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = (uint32_t)maxCount * m_ImageCount;

	VkDescriptorPoolSize poolSizes[5];
	uint32_t poolSizeCount = 0;

	if (storagePoolSize.descriptorCount > 0)
	{
		poolSizes[poolSizeCount++] = storagePoolSize;
	}
	if (dynamicStoragePoolSize.descriptorCount > 0)
	{
		poolSizes[poolSizeCount++] = dynamicStoragePoolSize;
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

#define ASSERT_RESULT_PIPELINE(exp)\
do\
{\
	if(!(exp))\
	{\
		pipeline->DebugDump();\
		assert(false && "assert failure please check");\
	}\
}\
while(false);

VkDescriptorSet KVulkanDescriptorPool::Alloc(size_t frameIndex, size_t currentFrame, IKPipeline* pipeline,
	const KDynamicConstantBufferUsage** ppConstantUsage, size_t dynamicBufferUsageCount,
	const KStorageBufferUsage** ppStorageUsage, size_t storageBufferUsageCount)
{
	KVulkanPipeline* vulkanPipeline = static_cast<KVulkanPipeline*>(pipeline);

	KVulkanDescriptorPoolAllocatedSetPtr allocation = InternalAlloc(frameIndex, currentFrame);

	VkDescriptorSet set = allocation->set;

	ASSERT_RESULT_PIPELINE(set != VK_NULL_HANDLE);

	ASSERT_RESULT_PIPELINE(!dynamicBufferUsageCount || ppConstantUsage);
	ASSERT_RESULT_PIPELINE(dynamicBufferUsageCount <= m_DynamicUniformBufferWriteInfo.size());
	ASSERT_RESULT_PIPELINE(dynamicBufferUsageCount <= m_DescriptorDynamicWriteInfo.size());

	size_t descriptorWriteIdx = 0;
	size_t hash = 0;

	if (vulkanPipeline)
	{
		auto& samplerBindings = vulkanPipeline->m_Samplers;
		ASSERT_RESULT_PIPELINE(samplerBindings.size() <= m_ImageWriteInfo.size());
		ASSERT_RESULT_PIPELINE(samplerBindings.size() <= m_DescriptorDynamicWriteInfo.size());

		auto& storageImageBindings = vulkanPipeline->m_StorageImages;
		ASSERT_RESULT_PIPELINE(storageImageBindings.size() <= m_StorageImageWriteInfo.size());
		ASSERT_RESULT_PIPELINE(storageImageBindings.size() <= m_DescriptorDynamicWriteInfo.size());

		auto& storageBufferBindings = vulkanPipeline->m_StorageBuffers;
		ASSERT_RESULT_PIPELINE(storageBufferBindings.size() <= m_StorageBufferWriteInfo.size());
		ASSERT_RESULT_PIPELINE(storageBufferBindings.size() <= m_DescriptorDynamicWriteInfo.size());

		size_t imageWriteIdx = 0;
		size_t storageImageWriteIdx = 0;
		size_t storageBufferWriteIdx = 0;
		size_t uniformBufferWriteIdx = 0;

		if (KVulkanGlobal::hashDescriptorUpdate)
		{
			for (auto& pair : samplerBindings)
			{
				unsigned int binding = pair.first;
				KVulkanPipeline::SamplerBindingInfo& info = pair.second;
				for (size_t i = 0; i < info.images.size(); ++i)
				{
					IKFrameBufferPtr frameBuffer = info.images[i];
					KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)frameBuffer.get();
					KHash::HashCombine(hash, ((KVulkanFrameBuffer*)frameBuffer.get())->GetUniqueID());
				}
				KHash::HashCombine(hash, binding);
			}

			for (auto& pair : storageImageBindings)
			{
				unsigned int binding = pair.first;
				KVulkanPipeline::StorageImageBindingInfo& info = pair.second;
				for (size_t i = 0; i < info.images.size(); ++i)
				{
					VkDescriptorImageInfo& imageInfo = m_StorageImageWriteInfo[i];
					IKFrameBufferPtr frameBuffer = info.images[i];
					KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)frameBuffer.get();
					KHash::HashCombine(hash, vulkanFrameBuffer->GetUniqueID());
				}
				KHash::HashCombine(hash, binding);
			}

			for (auto& pair : storageBufferBindings)
			{
				unsigned int binding = pair.first;
				KVulkanPipeline::StorageBufferBindingInfo& info = pair.second;
				KVulkanStorageBuffer* vulkanStorageBuffer = (KVulkanStorageBuffer*)info.buffer.get();
				KHash::HashCombine(hash, vulkanStorageBuffer->GetUniqueID());
				KHash::HashCombine(hash, binding);
			}
		}

		if (!KVulkanGlobal::hashDescriptorUpdate || allocation->hash0 != hash)
		{
			for (auto& pair : samplerBindings)
			{
				unsigned int binding = pair.first;
				KVulkanPipeline::SamplerBindingInfo& info = pair.second;

				VkDescriptorImageInfo& imageInfoStart = m_ImageWriteInfo[imageWriteIdx];

				assert(info.mipmaps.size() == info.images.size());

				for (size_t i = 0; i < info.images.size(); ++i)
				{
					VkDescriptorImageInfo& imageInfo = m_ImageWriteInfo[imageWriteIdx++];

					IKFrameBufferPtr frameBuffer = info.images[i];
					ASSERT_RESULT_PIPELINE(frameBuffer);

					KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)frameBuffer.get();

					uint32_t startMip = std::get<0>(info.mipmaps[i]);
					uint32_t numMip = std::get<1>(info.mipmaps[i]);
					if (startMip == 0 && numMip == 0)
					{
						imageInfo.imageView = vulkanFrameBuffer->GetImageView();
					}
					else
					{
						imageInfo.imageView = vulkanFrameBuffer->GetMipmapImageView(startMip, numMip);
					}

					imageInfo.imageLayout = frameBuffer->IsStorageImage() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo.sampler = ((KVulkanSampler*)info.samplers[0].get())->GetVkSampler();

					ASSERT_RESULT_PIPELINE(imageInfo.imageView);
					ASSERT_RESULT_PIPELINE(imageInfo.sampler);
				}

				VkWriteDescriptorSet& samplerDescriptorWrite = m_DescriptorDynamicWriteInfo[descriptorWriteIdx++];

				samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				samplerDescriptorWrite.dstSet = set;
				samplerDescriptorWrite.dstBinding = (uint32_t)binding;
				samplerDescriptorWrite.dstArrayElement = 0;
				samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				samplerDescriptorWrite.descriptorCount = (uint32_t)info.images.size();

				samplerDescriptorWrite.pBufferInfo = nullptr;
				samplerDescriptorWrite.pImageInfo = &imageInfoStart;
				samplerDescriptorWrite.pTexelBufferView = nullptr;
			}

			for (auto& pair : storageImageBindings)
			{
				unsigned int location = pair.first;
				KVulkanPipeline::StorageImageBindingInfo& info = pair.second;

				VkDescriptorImageInfo& imageInfoStart = m_StorageImageWriteInfo[storageImageWriteIdx];

				for (size_t i = 0; i < info.images.size(); ++i)
				{
					VkDescriptorImageInfo& imageInfo = m_StorageImageWriteInfo[storageImageWriteIdx++];
					IKFrameBufferPtr frameBuffer = info.images[i];
					KVulkanFrameBuffer* vulkanFrameBuffer = (KVulkanFrameBuffer*)frameBuffer.get();

					ASSERT_RESULT_PIPELINE(frameBuffer->IsStorageImage());

					imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageInfo.imageView = (info.format == EF_UNKNOWN) ? vulkanFrameBuffer->GetImageView() : vulkanFrameBuffer->GetReinterpretImageView(info.format);
					imageInfo.sampler = VK_NULL_HANDEL;

					KHash::HashCombine(hash, vulkanFrameBuffer->GetUniqueID());
				}

				VkWriteDescriptorSet& storageDescriptorWrite = m_DescriptorDynamicWriteInfo[descriptorWriteIdx++];

				storageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				// 写入的描述集合
				storageDescriptorWrite.dstSet = set;
				// 写入的位置 与DescriptorSetLayout里的VkDescriptorSetLayoutBinding位置对应
				storageDescriptorWrite.dstBinding = location;
				// 写入索引与下面descriptorCount对应
				storageDescriptorWrite.dstArrayElement = 0;

				storageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				storageDescriptorWrite.descriptorCount = (uint32_t)info.images.size();

				storageDescriptorWrite.pBufferInfo = nullptr;
				storageDescriptorWrite.pImageInfo = &imageInfoStart;
				storageDescriptorWrite.pTexelBufferView = nullptr;
			}

			for (auto& pair : storageBufferBindings)
			{
				unsigned int binding = pair.first;
				KVulkanPipeline::StorageBufferBindingInfo& info = pair.second;

				KVulkanStorageBuffer* vulkanStorageBuffer = (KVulkanStorageBuffer*)info.buffer.get();

				VkDescriptorBufferInfo& bufferInfo = m_StorageBufferWriteInfo[storageBufferWriteIdx++];
				bufferInfo.buffer = vulkanStorageBuffer->GetVulkanHandle();
				bufferInfo.offset = 0;
				bufferInfo.range = VK_WHOLE_SIZE;

				VkWriteDescriptorSet& storageDescriptorWrite = m_DescriptorDynamicWriteInfo[descriptorWriteIdx++];

				storageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				storageDescriptorWrite.dstSet = set;
				storageDescriptorWrite.dstBinding = (uint32_t)binding;
				storageDescriptorWrite.dstArrayElement = 0;
				storageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				storageDescriptorWrite.descriptorCount = 1;

				storageDescriptorWrite.pBufferInfo = &bufferInfo;
				storageDescriptorWrite.pImageInfo = nullptr;
				storageDescriptorWrite.pTexelBufferView = nullptr;
			}

			if (descriptorWriteIdx > 0)
			{
				vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(descriptorWriteIdx), m_DescriptorDynamicWriteInfo.data(), 0, nullptr);
			}
			allocation->hash0 = hash;
		}
	}

	descriptorWriteIdx = 0;
	hash = 0;

	for (size_t i = 0; i < dynamicBufferUsageCount; ++i)
	{
		const KDynamicConstantBufferUsage* usage = ppConstantUsage[i];

		IKUniformBufferPtr uniformBuffer = usage->buffer;

		VkDescriptorBufferInfo& bufferInfo = m_DynamicUniformBufferWriteInfo[i];
		bufferInfo.buffer = ((KVulkanUniformBuffer*)uniformBuffer.get())->GetVulkanHandle();
		// offset 不在这里绑定 在KVulkanCommandBuffer::Render时候绑定
		bufferInfo.offset = 0;
		bufferInfo.range = usage->range;

		VkWriteDescriptorSet& dynamicUniformDescriptorWrite = m_DescriptorDynamicWriteInfo[descriptorWriteIdx++];

		dynamicUniformDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		dynamicUniformDescriptorWrite.dstSet = set;
		dynamicUniformDescriptorWrite.dstBinding = (uint32_t)usage->binding;
		dynamicUniformDescriptorWrite.dstArrayElement = 0;
		dynamicUniformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		dynamicUniformDescriptorWrite.descriptorCount = 1;

		dynamicUniformDescriptorWrite.pBufferInfo = &bufferInfo;
		dynamicUniformDescriptorWrite.pImageInfo = nullptr;
		dynamicUniformDescriptorWrite.pTexelBufferView = nullptr;

		hash = HashCombine(dynamicUniformDescriptorWrite, hash);
	}

	for (size_t i = 0; i < storageBufferUsageCount; ++i)
	{
		const KStorageBufferUsage* usage = ppStorageUsage[i];

		IKVertexBufferPtr buffer = usage->buffer;

		VkDescriptorBufferInfo& bufferInfo = m_DynamicStorageBufferWriteInfo[i];
		bufferInfo.buffer = ((KVulkanVertexBuffer*)buffer.get())->GetVulkanHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet& storageDescriptorWrite = m_DescriptorDynamicWriteInfo[descriptorWriteIdx++];

		storageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		storageDescriptorWrite.dstSet = set;
		storageDescriptorWrite.dstBinding = (uint32_t)usage->binding;
		storageDescriptorWrite.dstArrayElement = 0;
		storageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		storageDescriptorWrite.descriptorCount = 1;

		storageDescriptorWrite.pBufferInfo = &bufferInfo;
		storageDescriptorWrite.pImageInfo = nullptr;
		storageDescriptorWrite.pTexelBufferView = nullptr;

		hash = HashCombine(storageDescriptorWrite, hash);
	}

	if ((!KVulkanGlobal::hashDescriptorUpdate || allocation->hash1 != hash) && descriptorWriteIdx > 0)
	{
		vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(descriptorWriteIdx), m_DescriptorDynamicWriteInfo.data(), 0, nullptr);
		allocation->hash1 = hash;
	}

	return set;
}

size_t KVulkanDescriptorPool::HashCombine(const VkWriteDescriptorSet& write, size_t currentHash)
{
	size_t hash = currentHash;
	if (KVulkanGlobal::hashDescriptorUpdate)
	{
		KHash::HashCombine(hash, write.dstBinding);
		KHash::HashCombine(hash, write.dstArrayElement);
		KHash::HashCombine(hash, write.descriptorCount);
		KHash::HashCombine(hash, write.descriptorType);

		if (write.pImageInfo)
		{
			for (uint32_t i = 0; i < write.descriptorCount; ++i)
			{
				KHash::HashCombine(hash, write.pImageInfo[i].sampler);
				KHash::HashCombine(hash, write.pImageInfo[i].imageView);
				KHash::HashCombine(hash, write.pImageInfo[i].imageLayout);
			}
		}

		if (write.pBufferInfo)
		{
			for (uint32_t i = 0; i < write.descriptorCount; ++i)
			{
				KHash::HashCombine(hash, write.pBufferInfo[i].buffer);
				KHash::HashCombine(hash, write.pBufferInfo[i].offset);
				KHash::HashCombine(hash, write.pBufferInfo[i].range);
			}
		}

		if (write.pTexelBufferView)
		{
			for (uint32_t i = 0; i < write.descriptorCount; ++i)
			{
				KHash::HashCombine(hash, write.pTexelBufferView[i]);
			}
		}
	}
	return hash;
}

KVulkanDescriptorPoolAllocatedSetPtr KVulkanDescriptorPool::InternalAlloc(size_t frameIndex, size_t currentFrame)
{
	ASSERT_RESULT(m_Layout != VK_NULL_HANDLE);

#ifdef _DEBUG
	KSectionEnterAssertGuard gurad(m_Allocating);
#endif

	if (frameIndex >= m_Descriptors.size())
	{
		m_Descriptors.resize(frameIndex + 1);
	}
	DescriptorSetBlockList& blockList = m_Descriptors[frameIndex];

	// 1.释放没用的Set
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
					vkFreeDescriptorSets(KVulkanGlobal::device, block.pool, 1, &block.sets[idx]->set);
					block.sets[idx]->hash0 = 0;
					block.sets[idx]->hash1 = 0;
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

	size_t blockIndex = 0;

	// 2.找没有超过Block预算的Set
	for (auto it = blockList.blocks.begin(), itEnd = blockList.blocks.end(); it != itEnd; ++it)
	{
		DescriptorSetBlock& block = *it;
		size_t newUseIndex = block.useCount++;
		if (newUseIndex < block.maxCount)
		{
			if (newUseIndex >= block.sets.size())
			{
				VkDescriptorSet newSet = AllocDescriptorSet(block.pool);
				if (!m_Name.empty())
				{
					KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)newSet, VK_OBJECT_TYPE_DESCRIPTOR_SET,
						(m_Name + "_DescriptorSet_" + std::to_string(blockIndex) + "_" + std::to_string(block.sets.size())).c_str());
				}

				KVulkanDescriptorPoolAllocatedSetPtr newAllocation(new KVulkanDescriptorPoolAllocatedSet(newSet));
				block.sets.push_back(newAllocation);
				return newAllocation;
			}
			else
			{
				return block.sets[newUseIndex];
			}
		}
		++blockIndex;
	}

	// 3.新建一个Block
	DescriptorSetBlock block;
	block.useCount = 0;
	block.maxCount = m_BlockSize;
	block.pool = CreateDescriptorPool(block.maxCount);

	VkDescriptorSet newSet = AllocDescriptorSet(block.pool);
	if (!m_Name.empty())
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)newSet, VK_OBJECT_TYPE_DESCRIPTOR_SET,
			(m_Name + "_DescriptorSet_" + std::to_string(blockIndex) + "_" + std::to_string(block.sets.size())).c_str());
	}

	KVulkanDescriptorPoolAllocatedSetPtr newAllocation(new KVulkanDescriptorPoolAllocatedSet(newSet));
	block.sets.push_back(newAllocation);
	++block.useCount;

	blockList.blocks.push_back(block);

	return newAllocation;
}