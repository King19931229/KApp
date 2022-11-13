#include "KVulkanComputePipeline.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanAccelerationStructure.h"
#include "KVulkanBuffer.h"
#include "KVulkanShader.h"
#include "KVulkanSampler.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "Internal/KRenderGlobal.h"

KVulkanComputePipeline::KVulkanComputePipeline()
	: m_DescriptorLayout(VK_NULL_HANDEL)
	, m_PipelineLayout(VK_NULL_HANDEL)
	, m_Pipeline(VK_NULL_HANDEL)
{
}

KVulkanComputePipeline::~KVulkanComputePipeline()
{
}

void KVulkanComputePipeline::BindSampler(uint32_t location, IKFrameBufferPtr target, IKSamplerPtr sampler, bool dynamicWrite)
{
	BindingInfo newBinding;
	newBinding.image.images = { target };
	newBinding.image.samplers = { sampler };
	newBinding.image.imageDescriptors = { {} };
	newBinding.image.format = EF_UNKNOWN;
	newBinding.flags = COMPUTE_RESOURCE_IN;
	newBinding.dynamicWrite = dynamicWrite;
	newBinding.type = BindingInfo::SAMPLER;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindSamplers(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, const std::vector<IKSamplerPtr>& samplers, bool dynamicWrite)
{
	BindingInfo newBinding;
	newBinding.image.images = targets;
	newBinding.image.samplers = samplers;
	newBinding.image.imageDescriptors.resize(targets.size());
	newBinding.image.format = EF_UNKNOWN;
	newBinding.flags = COMPUTE_RESOURCE_IN;
	newBinding.dynamicWrite = dynamicWrite;
	newBinding.type = BindingInfo::SAMPLER;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindStorageImage(uint32_t location, IKFrameBufferPtr target, ElementFormat format, ComputeResourceFlags flags, uint32_t mipmap, bool dynamicWrite)
{
	BindingInfo newBinding;
	newBinding.image.images = { target };
	newBinding.image.samplers = {};
	newBinding.image.imageDescriptors = { {} };
	newBinding.image.format = EF_UNKNOWN;
	newBinding.image.mipmap = mipmap;
	newBinding.image.format = format;
	newBinding.flags = flags;
	newBinding.dynamicWrite = dynamicWrite;
	newBinding.type = BindingInfo::IMAGE;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindStorageImages(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, ElementFormat format, ComputeResourceFlags flags, uint32_t mipmap, bool dynamicWrite)
{
	BindingInfo newBinding;
	newBinding.image.images = targets;
	newBinding.image.samplers = {};
	newBinding.image.imageDescriptors.resize(targets.size());
	newBinding.image.format = EF_UNKNOWN;
	newBinding.image.mipmap = mipmap;
	newBinding.image.format = format;
	newBinding.flags = flags;
	newBinding.dynamicWrite = dynamicWrite;
	newBinding.type = BindingInfo::IMAGE;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindStorageBuffer(uint32_t location, IKStorageBufferPtr buffer, ComputeResourceFlags flags, bool dynamicWrite)
{
	BindingInfo newBinding;
	newBinding.storage.buffer = buffer;
	newBinding.flags = flags;
	newBinding.dynamicWrite = dynamicWrite;
	newBinding.type = BindingInfo::STORAGE_BUFFER;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynamicWrite)
{
	BindingInfo newBinding;
	newBinding.as.as = as;
	newBinding.dynamicWrite = dynamicWrite;
	newBinding.type = BindingInfo::AS;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindUniformBuffer(uint32_t location, IKUniformBufferPtr buffer)
{
	BindingInfo newBinding;
	newBinding.uniform.buffer = buffer;
	newBinding.dynamicWrite = true;
	newBinding.type = BindingInfo::UNIFROM_BUFFER;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindDynamicUniformBuffer(uint32_t location)
{
	BindingInfo newBinding;
	newBinding.type = BindingInfo::DYNAMIC_UNIFROM_BUFFER;
	newBinding.dynamicWrite = true;
	m_Bindings[location] = newBinding;
}

VkWriteDescriptorSet KVulkanComputePipeline::PopulateImageWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding)
{
	auto& image = binding.image;
	VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

	if (binding.type == BindingInfo::SAMPLER)
	{
		type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ASSERT_RESULT(image.images.size() == image.samplers.size());
	}
	else
	{
		type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}

	for (size_t i = 0; i < image.images.size(); ++i)
	{
		IKFrameBufferPtr frameBuffer = image.images[i];
		KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(frameBuffer.get());

		if(type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			KVulkanSampler* vulkanSampler = static_cast<KVulkanSampler*>(image.samplers[i].get());
			type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			image.imageDescriptors[i] =
			{
				vulkanSampler->GetVkSampler(),
				image.format == EF_UNKNOWN ? vulkanFrameBuffer->GetImageView() : vulkanFrameBuffer->GetReinterpretImageView(image.format),
				vulkanFrameBuffer->GetImageLayout()
			};
		}
		else
		{
			image.imageDescriptors[i] =
			{
				VK_NULL_HANDLE,
				// TODO GetReinterpretImageView for mipmap
				image.format == EF_UNKNOWN ? vulkanFrameBuffer->GetMipmapImageView(image.mipmap) : vulkanFrameBuffer->GetReinterpretImageView(image.format),
				vulkanFrameBuffer->GetImageLayout()
			};
		}
	}

	return KVulkanInitializer::CreateDescriptorImageWrite(binding.image.imageDescriptors.data(), type, dstSet, dstBinding, (uint32_t)binding.image.images.size());
}

VkWriteDescriptorSet KVulkanComputePipeline::PopulateTopdownASWrite(BindingInfo & binding, VkDescriptorSet dstSet, uint32_t dstBinding)
{
	KVulkanAccelerationStructure* vulkanAccelerationStructure = static_cast<KVulkanAccelerationStructure*>(binding.as.as.get());
	binding.as.accelerationStructureDescriptor = {};
	binding.as.accelerationStructureDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	binding.as.accelerationStructureDescriptor.accelerationStructureCount = 1;
	binding.as.accelerationStructureDescriptor.pAccelerationStructures = &vulkanAccelerationStructure->GetTopDown().handle;
	return KVulkanInitializer::CreateDescriptorAccelerationStructureWrite(&binding.as.accelerationStructureDescriptor, dstSet, dstBinding, 1);
}

VkWriteDescriptorSet KVulkanComputePipeline::PopulateUniformBufferWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding)
{
	KVulkanUniformBuffer* vulkanUniformBuffer = static_cast<KVulkanUniformBuffer*>(binding.uniform.buffer.get());
	binding.uniform.bufferDescriptor = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanUniformBuffer->GetVulkanHandle(), 0, vulkanUniformBuffer->GetBufferSize());
	return KVulkanInitializer::CreateDescriptorBufferWrite(&binding.uniform.bufferDescriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, dstSet, dstBinding, 1);
}

VkWriteDescriptorSet KVulkanComputePipeline::PopulateStorageBufferWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding)
{
	KVulkanStorageBuffer* vulkanStorageBuffer = static_cast<KVulkanStorageBuffer*>(binding.storage.buffer.get());
	binding.storage.bufferDescriptor = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanStorageBuffer->GetVulkanHandle(), 0, vulkanStorageBuffer->GetBufferSize());
	return KVulkanInitializer::CreateDescriptorBufferWrite(&binding.storage.bufferDescriptor, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, dstSet, dstBinding, 1);
}

VkWriteDescriptorSet KVulkanComputePipeline::PopulateDynamicUniformBufferWrite(BindingInfo& binding, const KDynamicConstantBufferUsage& usage, VkDescriptorSet dstSet)
{
	KVulkanUniformBuffer* vulkanUniformBuffer = static_cast<KVulkanUniformBuffer*>(usage.buffer.get());
	uint32_t dstBinding = (uint32_t)usage.binding;
	binding.uniform.bufferDescriptor = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanUniformBuffer->GetVulkanHandle(), 0, usage.range);
	return KVulkanInitializer::CreateDescriptorBufferWrite(&binding.uniform.bufferDescriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, dstSet, dstBinding, 1);
}

VkDescriptorPool KVulkanComputePipeline::CreateDescriptorPool(uint32_t maxCount)
{
	VkDescriptorPool pool = VK_NULL_HANDEL;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = (uint32_t)m_PoolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = m_PoolSizes.data();
	descriptorPoolCreateInfo.maxSets = maxCount;
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VK_ASSERT_RESULT(vkCreateDescriptorPool(KVulkanGlobal::device, &descriptorPoolCreateInfo, nullptr, &pool));

	return pool;
}

VkDescriptorSet KVulkanComputePipeline::AllocDescriptorSet(VkDescriptorPool pool)
{
	VkDescriptorSet set = VK_NULL_HANDEL;

	std::vector<VkWriteDescriptorSet> staticWrites = m_StaticWrites;

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = pool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &m_DescriptorLayout;

	VK_ASSERT_RESULT(vkAllocateDescriptorSets(KVulkanGlobal::device, &descriptorSetAllocateInfo, &set));

	for (VkWriteDescriptorSet& write : staticWrites)
	{
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(staticWrites.size()), staticWrites.data(), 0, nullptr);

	return set;
}

VkDescriptorSet KVulkanComputePipeline::Alloc(size_t frameIndex, size_t frameNum)
{
	if(frameIndex < m_Descriptors.size())
	{
		DescriptorSetBlockList& blockList = m_Descriptors[frameIndex];

		if(blockList.currentFrame != frameNum)
		{
			blockList.currentFrame = frameNum;
			for(DescriptorSetBlock& block : blockList.blocks)
			{
				block.useCount = 0;
			}
		}

		for (DescriptorSetBlock& block : blockList.blocks)
		{
			uint32_t newUseIndex = block.useCount++;
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

		newBlock.pool = CreateDescriptorPool(newBlock.maxCount);
		VkDescriptorSet newSet = AllocDescriptorSet(newBlock.pool);
		newBlock.sets.push_back(newSet);
		++newBlock.useCount;

		blockList.blocks.push_back(newBlock);
		return newSet;
	}

	return VK_NULL_HANDEL;
}

void KVulkanComputePipeline::CreateLayout()
{
	uint32_t frames = KRenderGlobal::NumFramesInFlight;
	m_Descriptors.resize(frames);

	uint32_t stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	m_LayoutBindings.clear();
	m_PoolSizes.clear();
	m_StaticWrites.clear();

	m_LayoutBindings.reserve(m_Bindings.size());
	m_PoolSizes.reserve(m_Bindings.size());

	for (auto& pair : m_Bindings)
	{
		uint32_t location = pair.first;
		BindingInfo& binding = pair.second;

		VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		VkWriteDescriptorSet newWrite = {};

		if (binding.type == BindingInfo::SAMPLER)
		{
			type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			newWrite = PopulateImageWrite(binding, VK_NULL_HANDLE, location);
		}
		else if (binding.type == BindingInfo::IMAGE)
		{
			type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			newWrite = PopulateImageWrite(binding, VK_NULL_HANDLE, location);
		}
		else if (binding.type == BindingInfo::AS)
		{
			type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			newWrite = PopulateTopdownASWrite(binding, VK_NULL_HANDLE, location);
		}
		else if (binding.type == BindingInfo::UNIFROM_BUFFER)
		{
			type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			newWrite = PopulateUniformBufferWrite(binding, VK_NULL_HANDLE, location);
		}
		else if(binding.type == BindingInfo::STORAGE_BUFFER)
		{
			type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			newWrite = PopulateStorageBufferWrite(binding, VK_NULL_HANDLE, location);
		}
		else if (binding.type == BindingInfo::DYNAMIC_UNIFROM_BUFFER)
		{
			type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			// hack
			newWrite.descriptorCount = 1;
		}
		else
		{
			continue;
		}

		VkDescriptorSetLayoutBinding newBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(type, stageFlags, location, newWrite.descriptorCount);
		VkDescriptorPoolSize newPoolSize = { type, 1 };

		m_LayoutBindings.push_back(newBinding);
		m_PoolSizes.push_back(newPoolSize);

		if (!binding.dynamicWrite)
		{
			m_StaticWrites.push_back(newWrite);
		}
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {};
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCI.pBindings = m_LayoutBindings.data();
	descriptorSetLayoutCI.bindingCount = (uint32_t)m_LayoutBindings.size();

	VK_ASSERT_RESULT(vkCreateDescriptorSetLayout(KVulkanGlobal::device, &descriptorSetLayoutCI, nullptr, &m_DescriptorLayout));

	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &m_DescriptorLayout;

	VK_ASSERT_RESULT(vkCreatePipelineLayout(KVulkanGlobal::device, &pipelineLayoutCI, nullptr, &m_PipelineLayout));
}

void KVulkanComputePipeline::CreatePipeline()
{
	KVulkanShader* vulkanShader = (KVulkanShader*)(*m_ComputeShader).get();
	VkPipelineShaderStageCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderCreateInfo.module = vulkanShader->GetShaderModule();
	shaderCreateInfo.pSpecializationInfo = vulkanShader->GetSpecializationInfo();
	shaderCreateInfo.pName = "main";

	VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	createInfo.layout = m_PipelineLayout;
	createInfo.stage = shaderCreateInfo;

	vkCreateComputePipelines(KVulkanGlobal::device, KVulkanGlobal::pipelineCache, 1, &createInfo, nullptr, &m_Pipeline);
}

void KVulkanComputePipeline::DestroyDescriptorSet()
{
	for(DescriptorSetBlockList& blockList : m_Descriptors)
	{
		for (DescriptorSetBlock& block : blockList.blocks)
		{
			if (block.pool != VK_NULL_HANDEL)
			{
				vkDestroyDescriptorPool(KVulkanGlobal::device, block.pool, nullptr);
				block.pool = VK_NULL_HANDEL;
			}
		}
	}

	m_Descriptors.clear();

	if (m_DescriptorLayout != VK_NULL_HANDEL)
	{
		vkDestroyDescriptorSetLayout(KVulkanGlobal::device, m_DescriptorLayout, nullptr);
		m_DescriptorLayout = VK_NULL_HANDEL;
	}

	if (m_PipelineLayout != VK_NULL_HANDEL)
	{
		vkDestroyPipelineLayout(KVulkanGlobal::device, m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDEL;
	}
}

void KVulkanComputePipeline::DestroyPipeline()
{
	if (m_Pipeline != VK_NULL_HANDEL)
	{
		vkDestroyPipeline(KVulkanGlobal::device, m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDEL;
	}
}

bool KVulkanComputePipeline::Init(const char* szShader)
{
	UnInit();
	if (KRenderGlobal::ShaderManager.Acquire(ST_COMPUTE, szShader, m_ComputeShader, false))
	{
		CreateLayout();
		CreatePipeline();
		return true;
	}
	return false;
}

bool KVulkanComputePipeline::UnInit()
{
	DestroyPipeline();
	DestroyDescriptorSet();
	m_ComputeShader.Release();
	return true;
}

bool KVulkanComputePipeline::UpdateDynamicWrite(VkDescriptorSet dstSet, const KDynamicConstantBufferUsage* usage)
{
	std::vector<VkWriteDescriptorSet> dynamicWrites;
	dynamicWrites.reserve(m_Bindings.size());

	for (auto& pair : m_Bindings)
	{
		uint32_t location = pair.first;
		BindingInfo& binding = pair.second;

		if (!binding.dynamicWrite)
		{
			continue;
		}

		VkWriteDescriptorSet newWrite = {};

		if (binding.type == BindingInfo::SAMPLER)
		{
			newWrite = PopulateImageWrite(binding, dstSet, location);
		}
		else if (binding.type == BindingInfo::IMAGE)
		{
			newWrite = PopulateImageWrite(binding, dstSet, location);
		}
		else if (binding.type == BindingInfo::AS)
		{
			newWrite = PopulateTopdownASWrite(binding, dstSet, location);
		}
		else if (binding.type == BindingInfo::UNIFROM_BUFFER)
		{
			newWrite = PopulateUniformBufferWrite(binding, dstSet, location);
		}
		else if (binding.type == BindingInfo::STORAGE_BUFFER)
		{
			newWrite = PopulateStorageBufferWrite(binding, dstSet, location);
		}
		else if (binding.type == BindingInfo::DYNAMIC_UNIFROM_BUFFER)
		{
			ASSERT_RESULT(usage && usage->binding == location);
			newWrite = PopulateDynamicUniformBufferWrite(binding, *usage, dstSet);
		}
		else
		{
			continue;
		}

		dynamicWrites.push_back(newWrite);
	}

	if (dynamicWrites.size() > 0)
	{
		vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(dynamicWrites.size()), dynamicWrites.data(), 0, nullptr);
	}

	return true;
}

// https://gpuopen.com/learn/vulkan-barriers-explained/
bool KVulkanComputePipeline::SetupBarrier(IKCommandBufferPtr buffer, bool input)
{
	KVulkanCommandBuffer* commandBuffer = static_cast<KVulkanCommandBuffer*>(buffer.get());
	VkCommandBuffer cmdBuf = commandBuffer->GetVkHandle();

	std::vector<VkImageMemoryBarrier> imageBarriers;
	std::vector<VkBufferMemoryBarrier> buffBarriers;

	for (auto& pair : m_Bindings)
	{
		BindingInfo& binding = pair.second;

		if (input && !(binding.flags & COMPUTE_RESOURCE_IN))
		{
			continue;
		}
		if (!input && !(binding.flags & COMPUTE_RESOURCE_OUT))
		{
			continue;
		}

		for (size_t i = 0; i < binding.image.images.size(); ++i)
		{
			KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(binding.image.images[i].get());

			VkFormat format = vulkanFrameBuffer->GetForamt();
			VkImageAspectFlags flags = VK_IMAGE_ASPECT_COLOR_BIT;

			if (KVulkanHelper::HasDepthComponent(format))
			{
				flags = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (KVulkanHelper::HasStencilComponent(format))
				{
					flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}

			VkImageSubresourceRange range = { flags, 0, 1, 0, 1 };

			VkImageMemoryBarrier imgMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

			if (binding.type == BindingInfo::IMAGE)
			{
				if (input)
				{
					imgMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				}
				else
				{
					imgMemBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				}
			}
			else if(binding.type == BindingInfo::SAMPLER)
			{
				imgMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}

			imgMemBarrier.image = vulkanFrameBuffer->GetImage();
			imgMemBarrier.oldLayout = vulkanFrameBuffer->GetImageLayout();
			imgMemBarrier.newLayout = vulkanFrameBuffer->GetImageLayout();
			imgMemBarrier.subresourceRange = range;

			imageBarriers.push_back(imgMemBarrier);
		}

		if (binding.storage.buffer)
		{
			KVulkanStorageBuffer* vulkanStorageBuffer = static_cast<KVulkanStorageBuffer*>(binding.storage.buffer.get());

			VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufMemBarrier.size = vulkanStorageBuffer->GetBufferSize();
			bufMemBarrier.buffer = vulkanStorageBuffer->GetVulkanHandle();
			bufMemBarrier.offset = 0;

			if (input)
			{
				bufMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				bufMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}
			else
			{
				bufMemBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				bufMemBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			}

			bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			buffBarriers.push_back(bufMemBarrier);
		}
	}

	VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	vkCmdPipelineBarrier(cmdBuf, stageFlags, stageFlags, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, (uint32_t)buffBarriers.size(), buffBarriers.data(), (uint32_t)imageBarriers.size(), imageBarriers.data());

	return true;
}

void KVulkanComputePipeline::PreDispatch(IKCommandBufferPtr primaryBuffer, VkDescriptorSet dstSet, const KDynamicConstantBufferUsage* usage)
{
	KVulkanCommandBuffer* commandBuffer = static_cast<KVulkanCommandBuffer*>(primaryBuffer.get());
	VkCommandBuffer cmdBuf = commandBuffer->GetVkHandle();

	uint32_t dynamicOffsets = 0;
	uint32_t dynamicBufferCount = 0;

	if (usage)
	{
		dynamicOffsets = (uint32_t)usage->offset;
		dynamicBufferCount = 1;
	}

	// Update the descriptor
	UpdateDynamicWrite(dstSet, usage);

	// Adding a barrier to be sure the fragment has finished writing
	SetupBarrier(primaryBuffer, true);

	// Preparing for the compute shader
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &dstSet, dynamicBufferCount, &dynamicOffsets);

}

void KVulkanComputePipeline::PostDispatch(IKCommandBufferPtr primaryBuffer)
{
	// Adding a barrier to be sure the compute shader has finished
	SetupBarrier(primaryBuffer, false);
}

bool KVulkanComputePipeline::Execute(IKCommandBufferPtr primaryBuffer, uint32_t groupX, uint32_t groupY, uint32_t groupZ, const KDynamicConstantBufferUsage* usage)
{
	uint32_t frameIndex = KRenderGlobal::CurrentFrameIndex;
	if (primaryBuffer && frameIndex < m_Descriptors.size())
	{
		VkDescriptorSet dstSet = Alloc(frameIndex, KRenderGlobal::CurrentFrameNum);

		PreDispatch(primaryBuffer, dstSet, usage);

		// Dispatching the shader
		KVulkanCommandBuffer* commandBuffer = static_cast<KVulkanCommandBuffer*>(primaryBuffer.get());
		VkCommandBuffer cmdBuf = commandBuffer->GetVkHandle();
		vkCmdDispatch(cmdBuf, groupX, groupY, groupZ);

		PostDispatch(primaryBuffer);

		return true;
	}
	return false;
}

bool KVulkanComputePipeline::ExecuteIndirect(IKCommandBufferPtr primaryBuffer, IKStorageBufferPtr indirectBuffer, const KDynamicConstantBufferUsage* usage)
{
	uint32_t frameIndex = KRenderGlobal::CurrentFrameIndex;
	if (primaryBuffer && frameIndex < m_Descriptors.size())
	{
		VkDescriptorSet dstSet = Alloc(frameIndex, KRenderGlobal::CurrentFrameNum);

		PreDispatch(primaryBuffer, dstSet, usage);

		// Dispatching the shader
		KVulkanCommandBuffer* commandBuffer = static_cast<KVulkanCommandBuffer*>(primaryBuffer.get());
		VkCommandBuffer cmdBuf = commandBuffer->GetVkHandle();

		KVulkanStorageBuffer* storageBuffer = static_cast<KVulkanStorageBuffer*>(indirectBuffer.get());
		VkBuffer indirectBuf = storageBuffer->GetVulkanHandle();
		VkDeviceSize indirectSize = storageBuffer->GetBufferSize();

		// Setup the indirect buffer barrier
		{
			VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufMemBarrier.size = indirectSize;
			bufMemBarrier.buffer = indirectBuf;
			bufMemBarrier.offset = 0;

			bufMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			bufMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

			bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			vkCmdPipelineBarrier(cmdBuf, stageFlags, stageFlags, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 1, &bufMemBarrier, 0, nullptr);
		}

		vkCmdDispatchIndirect(cmdBuf, indirectBuf, 0);

		PostDispatch(primaryBuffer);

		return true;
	}
	return false;
}

bool KVulkanComputePipeline::Reload()
{
	if (m_ComputeShader && (*m_ComputeShader)->Reload())
	{
		DestroyPipeline();
		CreatePipeline();
		return true;
	}
	return false;
}