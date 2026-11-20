#include "KVulkanDescripiorPool.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanPipeline.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "KVulkanHeapAllocator.h"
#include "KVulkanInitializer.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KHash.h"
#include "KBase/Publish/KSectionEnterAssertGuard.h"
#include <algorithm>
#include <limits>

static VkDescriptorPool CreateNativeDescriptorPool(const std::vector<VkDescriptorPoolSize>& sizes, uint32_t count, VkResult& result)
{
	std::vector<VkDescriptorPoolSize> poolSizes = sizes;
	for (VkDescriptorPoolSize& size : poolSizes)
	{
		if ((uint64_t)size.descriptorCount * count > (std::numeric_limits<uint32_t>::max)())
		{
			result = VK_ERROR_OUT_OF_POOL_MEMORY;
			return VK_NULL_HANDLE;
		}
		size.descriptorCount *= count;
	}
	VkDescriptorPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.poolSizeCount = (uint32_t)poolSizes.size();
	info.pPoolSizes = poolSizes.data();
	info.maxSets = count;
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	result = vkCreateDescriptorPool(KVulkanGlobal::device, &info, nullptr, &pool);
	if (KVulkanIsOOM(result) && KVulkanHeapAllocator::ReleaseUnusedPages())
	{
		result = vkCreateDescriptorPool(KVulkanGlobal::device, &info, nullptr, &pool);
	}
	return result == VK_SUCCESS ? pool : VK_NULL_HANDLE;
}

KVulkanDescriptorArena::KVulkanDescriptorArena(const std::vector<VkDescriptorPoolSize>& poolSizes)
	: m_PoolSizes(poolSizes)
{
}

KVulkanDescriptorArena::~KVulkanDescriptorArena()
{
	ASSERT_RESULT(m_Pages.empty());
}

VkDescriptorPool KVulkanDescriptorArena::Acquire(uint32_t count)
{
	if (!count) return VK_NULL_HANDLE;
	for (Page& page : m_Pages)
	{
		if (page.capacity - page.reserved >= count)
		{
			page.reserved += count;
			return page.pool;
		}
	}
	uint32_t capacity = std::max(count, m_NextPageSize);
	for (const VkDescriptorPoolSize& size : m_PoolSizes)
	{
		if (size.descriptorCount)
		{
			capacity = std::min(capacity, (std::numeric_limits<uint32_t>::max)() / size.descriptorCount);
		}
	}
	VkResult result = VK_ERROR_OUT_OF_POOL_MEMORY;
	VkDescriptorPool pool = capacity >= count ? CreateNativeDescriptorPool(m_PoolSizes, capacity, result) : VK_NULL_HANDLE;
	if (!pool && capacity > count)
	{
		capacity = count;
		pool = CreateNativeDescriptorPool(m_PoolSizes, capacity, result);
	}
	if (!pool)
	{
		VK_ASSERT_RESULT_WITH_CALL(result, "vkCreateDescriptorPool");
		return VK_NULL_HANDLE;
	}
	m_Pages.push_back({ pool, capacity, count });
	m_NextPageSize = (uint32_t)std::min(uint64_t(512), uint64_t(capacity) * 2);
	return pool;
}

void KVulkanDescriptorArena::Release(VkDescriptorPool pool, uint32_t count)
{
	for (auto it = m_Pages.begin(); it != m_Pages.end(); ++it)
	{
		if (it->pool != pool) continue;
		ASSERT_RESULT(count <= it->reserved);
		if (count > it->reserved) return;
		it->reserved -= count;
		if (!it->reserved)
		{
			vkDestroyDescriptorPool(KVulkanGlobal::device, pool, nullptr);
			m_Pages.erase(it);
			if (m_Pages.empty())
			{
				m_NextPageSize = 8;
			}
		}
		return;
	}
	assert(false && "descriptor pool reservation not found");
}

bool KVulkanDescriptorArena::Matches(const std::vector<VkDescriptorPoolSize>& sizes) const
{
	if (sizes.size() != m_PoolSizes.size()) return false;
	for (const VkDescriptorPoolSize& size : sizes)
	{
		const bool found = std::any_of(m_PoolSizes.begin(), m_PoolSizes.end(), [&size](const VkDescriptorPoolSize& other)
		{
			return size.type == other.type && size.descriptorCount == other.descriptorCount;
		});
		if (!found)
		{
			return false;
		}
	}
	return true;
}

KVulkanDescriptorPool::KVulkanDescriptorPool()
	: m_Layout(VK_NULL_HANDLE)
	, m_Trim(false)
	, m_BlockSize(512)
#ifdef _DEBUG
	, m_Allocating(false)
#endif
{
}

KVulkanDescriptorPool::~KVulkanDescriptorPool()
{
	ASSERT_RESULT(m_Descriptors.empty());
}

bool KVulkanDescriptorPool::Init(VkDescriptorSetLayout layout,
	const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t blockSize, bool trim)
{
#ifdef _DEBUG
	KSectionEnterAssertGuard guard(m_Allocating);
#endif
	UnInit();

	ASSERT_RESULT(layout);

	m_Layout = layout;
	m_PoolSizes = poolSizes;
	m_BlockSize = blockSize;
	m_Trim = trim;
	return true;
}

KVulkanGraphicsDescriptorWriter::KVulkanGraphicsDescriptorWriter()
	: m_ImageCount(0)
	, m_StorageImageCount(0)
	, m_UniformBufferCount(0)
	, m_StorageBufferCount(0)
	, m_DynamicUniformBufferCount(0)
	, m_DynamicStorageBufferCount(0)
{
}

bool KVulkanGraphicsDescriptorWriter::Init(const std::vector<VkDescriptorPoolSize>& poolSizes)
{
	m_ImageCount = 0;
	m_StorageImageCount = 0;
	m_UniformBufferCount = 0;
	m_StorageBufferCount = 0;
	m_DynamicUniformBufferCount = 0;
	m_DynamicStorageBufferCount = 0;

	for (const VkDescriptorPoolSize& poolSize : poolSizes)
	{
		if (poolSize.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			m_ImageCount += poolSize.descriptorCount;
		}
		else if (poolSize.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			m_StorageImageCount += poolSize.descriptorCount;
		}
		else if (poolSize.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			m_StorageBufferCount += poolSize.descriptorCount;
		}
		else if (poolSize.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
			m_DynamicUniformBufferCount += poolSize.descriptorCount;
		}
		else if (poolSize.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
		{
			m_DynamicStorageBufferCount += poolSize.descriptorCount;
		}
		else if (poolSize.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			m_UniformBufferCount += poolSize.descriptorCount;
		}
	}

	m_ImageWriteInfo.resize(m_ImageCount);
	m_StorageImageWriteInfo.resize(m_StorageImageCount);
	m_StorageBufferWriteInfo.resize(m_StorageBufferCount);
	m_UniformBufferWriteInfo.resize(m_UniformBufferCount);
	m_DynamicUniformBufferWriteInfo.resize(m_DynamicUniformBufferCount);
	m_DynamicStorageBufferWriteInfo.resize(m_DynamicStorageBufferCount);

	m_DescriptorDynamicWriteInfo.resize(m_ImageCount + m_StorageImageCount + m_StorageBufferCount + m_UniformBufferCount + m_DynamicUniformBufferCount + m_DynamicStorageBufferCount);

	return true;
}

bool KVulkanDescriptorPool::UnInit()
{
	for (DescriptorSetBlockList& blockList : m_Descriptors)
	{
		Clear(blockList);
	}
	m_Descriptors.clear();
	m_ArenaProvider = nullptr;

	m_Layout = VK_NULL_HANDLE;
	return true;
}

void KVulkanDescriptorPool::DestroyDescriptorPool(DescriptorSetBlockList& list, DescriptorSetBlock& block)
{
	if (list.arena)
	{
		std::vector<VkDescriptorSet> sets;
		sets.reserve(block.sets.size());
		for (const KVulkanDescriptorPoolAllocatedSetPtr& allocation : block.sets)
		{
			sets.push_back(allocation->set);
		}
		if (!sets.empty())
		{
			VK_ASSERT_RESULT(vkFreeDescriptorSets(KVulkanGlobal::device, block.pool, (uint32_t)sets.size(), sets.data()));
		}
		list.arena->Release(block.pool, block.maxCount);
	}
	else
	{
		vkDestroyDescriptorPool(KVulkanGlobal::device, block.pool, nullptr);
	}
	block.pool = VK_NULL_HANDLE;
	block.sets.clear();
}

void KVulkanDescriptorPool::Clear(DescriptorSetBlockList& list)
{
	for (DescriptorSetBlock& block : list.blocks)
	{
		DestroyDescriptorPool(list, block);
	}
	list.blocks.clear();
	list.arena.reset();
	list.occupied = false;
}

void KVulkanDescriptorPool::ReleaseCommandPool(VkCommandPool commandPool)
{
	for (auto it = m_Descriptors.begin(); it != m_Descriptors.end();)
	{
		if (it->commandPool == commandPool)
		{
			Clear(*it);
			it = m_Descriptors.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void KVulkanDescriptorPool::AppendDescriptorSets(std::vector<VkDescriptorSet>& sets) const
{
	for (const DescriptorSetBlockList& blockList : m_Descriptors)
	{
		for (const DescriptorSetBlock& block : blockList.blocks)
		{
			for (const KVulkanDescriptorPoolAllocatedSetPtr& allocation : block.sets)
			{
				sets.push_back(allocation->set);
			}
		}
	}
}

VkDescriptorSet KVulkanDescriptorPool::AllocDescriptorSet(VkDescriptorPool pool)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_Layout;

	VkDescriptorSet newSet = VK_NULL_HANDLE;
	VkResult result = vkAllocateDescriptorSets(KVulkanGlobal::device, &allocInfo, &newSet);
	if (KVulkanIsOOM(result) && KVulkanHeapAllocator::ReleaseUnusedPages())
	{
		result = vkAllocateDescriptorSets(KVulkanGlobal::device, &allocInfo, &newSet);
	}
	if (result != VK_SUCCESS)
	{
		VK_ASSERT_RESULT_WITH_CALL(result, "vkAllocateDescriptorSets");
		return VK_NULL_HANDLE;
	}

	return newSet;
}

VkDescriptorPool KVulkanDescriptorPool::CreateDescriptorPool(uint32_t maxCount)
{
	VkResult result;
	VkDescriptorPool newPool = CreateNativeDescriptorPool(m_PoolSizes, maxCount, result);
	if (result != VK_SUCCESS)
	{
		VK_ASSERT_RESULT_WITH_CALL(result, "vkCreateDescriptorPool");
		return VK_NULL_HANDLE;
	}
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

VkDescriptorSet KVulkanGraphicsDescriptorWriter::UpdateDescriptorSet(KVulkanPipeline* pipeline,
	const KVulkanDescriptorPoolAllocatedSetPtr& allocation, bool newSet,
	const KDynamicConstantBufferUsage** ppConstantUsage, uint32_t dynamicBufferUsageCount,
	const KStorageBufferUsage** ppStorageUsage, uint32_t storageBufferUsageCount)
{
	if (!allocation || !allocation->set) return VK_NULL_HANDLE;
	KVulkanPipeline* vulkanPipeline = pipeline;

	VkDescriptorSet set = allocation->set;

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

		auto& uniformBufferBindings = vulkanPipeline->m_Uniforms;
		ASSERT_RESULT_PIPELINE(uniformBufferBindings.size() <= m_UniformBufferWriteInfo.size());
		ASSERT_RESULT_PIPELINE(uniformBufferBindings.size() <= m_DescriptorDynamicWriteInfo.size());

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
					KHash::HashCombine(hash, vulkanFrameBuffer->GetUniqueID());
					KHash::HashCombine(hash, ((KVulkanSampler*)info.samplers[i].get())->GetVkSampler());
					KHash::HashCombine(hash, std::get<0>(info.mipmaps[i]));
					KHash::HashCombine(hash, std::get<1>(info.mipmaps[i]));
				}
				KHash::HashCombine(hash, binding);
			}

			for (auto& pair : storageImageBindings)
			{
				unsigned int binding = pair.first;
				KVulkanPipeline::StorageImageBindingInfo& info = pair.second;
				KHash::HashCombine(hash, info.format);
				for (size_t i = 0; i < info.images.size(); ++i)
				{
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

			for (auto& pair : uniformBufferBindings)
			{
				unsigned int binding = pair.first;
				KVulkanPipeline::UniformBufferBindingInfo& info = pair.second;
				KVulkanUniformBuffer* vulkanUniformBuffer = (KVulkanUniformBuffer*)info.buffer.get();
				KHash::HashCombine(hash, vulkanUniformBuffer->GetUniqueID());
				KHash::HashCombine(hash, vulkanUniformBuffer->GetBufferSize());
				KHash::HashCombine(hash, binding);
			}
		}

		if (newSet || !KVulkanGlobal::hashDescriptorUpdate || allocation->hash0 != hash)
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
					imageInfo.sampler = ((KVulkanSampler*)info.samplers[i].get())->GetVkSampler();

					ASSERT_RESULT_PIPELINE(imageInfo.imageView);
					ASSERT_RESULT_PIPELINE(imageInfo.sampler);
				}

				m_DescriptorDynamicWriteInfo[descriptorWriteIdx++] = KVulkanInitializer::CreateDescriptorImageWrite(
					&imageInfoStart, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, set, binding, (uint32_t)info.images.size());
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
					imageInfo.imageView = (info.format == PF_UNKNOWN) ? vulkanFrameBuffer->GetImageView() : vulkanFrameBuffer->GetReinterpretImageView(info.format);
					imageInfo.sampler = VK_NULL_HANDEL;
				}

				m_DescriptorDynamicWriteInfo[descriptorWriteIdx++] = KVulkanInitializer::CreateDescriptorImageWrite(
					&imageInfoStart, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, set, location, (uint32_t)info.images.size());
			}

			for (auto& pair : storageBufferBindings)
			{
				unsigned int binding = pair.first;
				KVulkanPipeline::StorageBufferBindingInfo& info = pair.second;

				KVulkanStorageBuffer* vulkanStorageBuffer = (KVulkanStorageBuffer*)info.buffer.get();

				VkDescriptorBufferInfo& bufferInfo = m_StorageBufferWriteInfo[storageBufferWriteIdx++];
				bufferInfo = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanStorageBuffer->GetVulkanHandle());

				m_DescriptorDynamicWriteInfo[descriptorWriteIdx++] = KVulkanInitializer::CreateDescriptorBufferWrite(
					&bufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, set, binding);
			}

			for (auto& pair : uniformBufferBindings)
			{
				uint32_t binding = pair.first;
				KVulkanUniformBuffer* vulkanUniformBuffer = (KVulkanUniformBuffer*)pair.second.buffer.get();

				VkDescriptorBufferInfo& bufferWrite = m_UniformBufferWriteInfo[uniformBufferWriteIdx++];
				bufferWrite = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanUniformBuffer->GetVulkanHandle(), 0, vulkanUniformBuffer->GetBufferSize());

				m_DescriptorDynamicWriteInfo[descriptorWriteIdx++] = KVulkanInitializer::CreateDescriptorBufferWrite(
					&bufferWrite, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, set, binding);
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
		dynamicUniformDescriptorWrite = KVulkanInitializer::CreateDescriptorBufferWrite(
			&bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, set, (uint32_t)usage->binding);

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
		storageDescriptorWrite = KVulkanInitializer::CreateDescriptorBufferWrite(
			&bufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, set, (uint32_t)usage->binding);

		hash = HashCombine(storageDescriptorWrite, hash);
	}

	if ((newSet || !KVulkanGlobal::hashDescriptorUpdate || allocation->hash1 != hash) && descriptorWriteIdx > 0)
	{
		vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(descriptorWriteIdx), m_DescriptorDynamicWriteInfo.data(), 0, nullptr);
		allocation->hash1 = hash;
	}

	return set;
}

size_t KVulkanGraphicsDescriptorWriter::HashCombine(const VkWriteDescriptorSet& write, size_t currentHash)
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

void KVulkanDescriptorPool::Release(uint32_t frameNum)
{
	for (DescriptorSetBlockList& blockList : m_Descriptors)
	{
		if (!blockList.occupied || blockList.currentFrame != frameNum)
		{
			continue;
		}

		for (auto it = blockList.blocks.begin(); it != blockList.blocks.end();)
		{
			DescriptorSetBlock& block = *it;
			if (m_Trim && block.useCount <= block.sets.size() / 2)
			{
				size_t newCount = block.sets.size() / 2;
				if (!newCount)
				{
					DestroyDescriptorPool(blockList, block);
					it = blockList.blocks.erase(it);
					continue;
				}
				while (block.sets.size() > newCount)
				{
					VkResult result = vkFreeDescriptorSets(KVulkanGlobal::device, block.pool, 1, &block.sets.back()->set);
					if (result != VK_SUCCESS)
					{
						VK_ASSERT_RESULT(result);
						break;
					}
					block.sets.pop_back();
				}
			}
			block.useCount = 0;
			++it;
		}
		blockList.occupied = false;
	}
}

bool KVulkanDescriptorPool::IsOccupied() const
{
	for (const DescriptorSetBlockList& blockList : m_Descriptors)
	{
		if (blockList.occupied) return true;
	}
	return false;
}

KVulkanDescriptorPoolAllocatedSetPtr KVulkanDescriptorPool::Alloc(bool& newSet, VkCommandPool commandPool)
{
	return InternalAlloc(false, newSet, commandPool);
}

KVulkanDescriptorPoolAllocatedSetPtr KVulkanDescriptorPool::Request(bool& newSet)
{
	return InternalAlloc(true, newSet, VK_NULL_HANDLE);
}

KVulkanDescriptorPoolAllocatedSetPtr KVulkanDescriptorPool::InternalAlloc(bool reuseSet, bool& newSet, VkCommandPool commandPool)
{
	ASSERT_RESULT(m_Layout != VK_NULL_HANDLE);

#ifdef _DEBUG
	KSectionEnterAssertGuard guard(m_Allocating);
#endif

	newSet = false;
	uint32_t frameNum = KRenderGlobal::CurrentFrameNum;
	DescriptorSetBlockList* list = nullptr;
	for (DescriptorSetBlockList& candidate : m_Descriptors)
	{
		if (candidate.commandPool != commandPool) continue;
		if (candidate.occupied && candidate.currentFrame == frameNum)
		{
			list = &candidate;
			break;
		}
		if (!candidate.occupied && !list)
		{
			list = &candidate;
		}
	}
	if (!list)
	{
		m_Descriptors.emplace_back();
		list = &m_Descriptors.back();
		list->commandPool = commandPool;
	}

	DescriptorSetBlockList& blockList = *list;
	if (commandPool != VK_NULL_HANDLE && !blockList.arena)
	{
		if (!m_ArenaProvider) return nullptr;
		blockList.arena = m_ArenaProvider(commandPool);
		if (!blockList.arena) return nullptr;
	}

	size_t blockIndex = 0;
	if (!reuseSet)
	{
		while (blockIndex < blockList.blocks.size() && blockList.blocks[blockIndex].useCount == blockList.blocks[blockIndex].maxCount)
		{
			++blockIndex;
		}
	}

	const bool newBlock = blockIndex == blockList.blocks.size();
	if (newBlock)
	{
		DescriptorSetBlock block;
		block.maxCount = std::min(m_BlockSize, blockList.blocks.empty() ? 1u : blockList.blocks.back().maxCount * 2);
		block.pool = blockList.arena ? blockList.arena->Acquire(block.maxCount) : CreateDescriptorPool(block.maxCount);
		if (!block.pool) return nullptr;
		blockList.blocks.push_back(std::move(block));
	}
	DescriptorSetBlock& block = blockList.blocks[blockIndex];
	const uint32_t setIndex = reuseSet ? 0 : block.useCount;
	if (setIndex == block.sets.size())
	{
		VkDescriptorSet set = AllocDescriptorSet(block.pool);
		if (!set)
		{
			if (newBlock)
			{
				DestroyDescriptorPool(blockList, block);
				blockList.blocks.pop_back();
			}
			return nullptr;
		}
		if (!m_Name.empty())
		{
			KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)set, VK_OBJECT_TYPE_DESCRIPTOR_SET,
				(m_Name + "_DescriptorSet(Block" + std::to_string(blockIndex) + " Set" + std::to_string(setIndex) + ")").c_str());
		}
		block.sets.emplace_back(KNEW KVulkanDescriptorPoolAllocatedSet(set, commandPool));
		newSet = true;
	}
	block.useCount = setIndex + 1;
	blockList.currentFrame = frameNum;
	blockList.occupied = true;
	return block.sets[setIndex];
}