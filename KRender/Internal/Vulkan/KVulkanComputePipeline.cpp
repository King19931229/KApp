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
#include "Internal/KRenderGlobal.h"

KVulkanComputePipeline::KVulkanComputePipeline()
{
}

KVulkanComputePipeline::~KVulkanComputePipeline()
{
}

void KVulkanComputePipeline::BindSampler(uint32_t location, IKFrameBufferPtr target, IKSamplerPtr sampler, bool dynimicWrite)
{
	BindingInfo newBinding;
	newBinding.image.images = { target };
	newBinding.image.samplers = { sampler };
	newBinding.image.imageDescriptors = { {} };
	newBinding.image.flag = COMPUTE_IMAGE_IN;
	newBinding.image.format = EF_UNKNOWN;
	newBinding.dynamicWrite = dynimicWrite;
	newBinding.type = BindingInfo::SAMPLER;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindSamplers(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, const std::vector<IKSamplerPtr>& samplers, bool dynimicWrite)
{
	BindingInfo newBinding;
	newBinding.image.images = targets;
	newBinding.image.samplers = samplers;
	newBinding.image.imageDescriptors.resize(targets.size());
	newBinding.image.flag = COMPUTE_IMAGE_IN;
	newBinding.image.format = EF_UNKNOWN;
	newBinding.dynamicWrite = dynimicWrite;
	newBinding.type = BindingInfo::SAMPLER;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindStorageImage(uint32_t location, IKFrameBufferPtr target, ComputeImageFlag flag, bool dynimicWrite)
{
	BindingInfo newBinding;
	newBinding.image.images = { target };
	newBinding.image.samplers = {};
	newBinding.image.imageDescriptors = { {} };
	newBinding.image.flag = flag;
	newBinding.image.format = EF_UNKNOWN;
	newBinding.dynamicWrite = dynimicWrite;
	newBinding.type = BindingInfo::IMAGE;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindStorageImages(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, ComputeImageFlag flag, bool dynimicWrite)
{
	BindingInfo newBinding;
	newBinding.image.images = targets;
	newBinding.image.samplers = {};
	newBinding.image.imageDescriptors.resize(targets.size());
	newBinding.image.flag = flag;
	newBinding.image.format = EF_UNKNOWN;
	newBinding.dynamicWrite = dynimicWrite;
	newBinding.type = BindingInfo::IMAGE;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynimicWrite)
{
	BindingInfo newBinding;
	newBinding.as.as = as;
	newBinding.dynamicWrite = dynimicWrite;
	newBinding.type = BindingInfo::AS;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindUniformBuffer(uint32_t location, IKUniformBufferPtr buffer, bool dynimicWrite)
{
	BindingInfo newBinding;
	newBinding.buffer.buffer = buffer;
	newBinding.dynamicWrite = dynimicWrite;
	newBinding.type = BindingInfo::BUFFER;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::BindDyanmicUniformBuffer(uint32_t location)
{
	BindingInfo newBinding;
	newBinding.type = BindingInfo::DYNAMIC_BUFFER;
	newBinding.dynamicWrite = true;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::ReinterpretImageFormat(uint32_t location, ElementFormat format)
{
	auto it = m_Bindings.find(location);
	if (it != m_Bindings.end())
	{
		BindingInfo& binding = it->second;
		binding.image.format = format;
	}
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
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
		}
		else
		{
			image.imageDescriptors[i] =
			{
				VK_NULL_HANDLE,
				image.format == EF_UNKNOWN ? vulkanFrameBuffer->GetImageView() : vulkanFrameBuffer->GetReinterpretImageView(image.format),
				VK_IMAGE_LAYOUT_GENERAL
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
	KVulkanUniformBuffer* vulkanUniformBuffer = static_cast<KVulkanUniformBuffer*>(binding.buffer.buffer.get());
	binding.buffer.bufferDescriptor = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanUniformBuffer->GetVulkanHandle(), 0, vulkanUniformBuffer->GetBufferSize());
	return KVulkanInitializer::CreateDescriptorBufferWrite(&binding.buffer.bufferDescriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, dstSet, dstBinding, 1);
}

VkWriteDescriptorSet KVulkanComputePipeline::PopulateDynamicUniformBufferWrite(BindingInfo& binding, const KDynamicConstantBufferUsage& usage, VkDescriptorSet dstSet)
{
	KVulkanUniformBuffer* vulkanUniformBuffer = static_cast<KVulkanUniformBuffer*>(usage.buffer.get());
	uint32_t dstBinding = (uint32_t)usage.binding;
	binding.buffer.bufferDescriptor = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanUniformBuffer->GetVulkanHandle(), 0, usage.range);
	return KVulkanInitializer::CreateDescriptorBufferWrite(&binding.buffer.bufferDescriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, dstSet, dstBinding, 1);
}

void KVulkanComputePipeline::CreateDescriptorSet()
{
	uint32_t frames = KRenderGlobal::RenderDevice->GetNumFramesInFlight();
	m_Descriptor.sets.resize(frames);

	uint32_t stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	std::vector<VkDescriptorPoolSize> poolSizes;

	std::vector<VkWriteDescriptorSet> staticWrites;

	layoutBindings.reserve(m_Bindings.size());
	poolSizes.reserve(m_Bindings.size());

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
		else if (binding.type == BindingInfo::BUFFER)
		{
			type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			newWrite = PopulateUniformBufferWrite(binding, VK_NULL_HANDLE, location);
		}
		else if (binding.type == BindingInfo::DYNAMIC_BUFFER)
		{
			type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			newWrite.descriptorCount = 1;
		}
		else
		{
			continue;
		}

		VkDescriptorSetLayoutBinding newBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(type, stageFlags, location, newWrite.descriptorCount);
		VkDescriptorPoolSize newPoolSize = { type, 1 };

		layoutBindings.push_back(newBinding);
		poolSizes.push_back(newPoolSize);

		if (!binding.dynamicWrite)
		{
			staticWrites.push_back(newWrite);
		}
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {};
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCI.pBindings = layoutBindings.data();
	descriptorSetLayoutCI.bindingCount = (uint32_t)layoutBindings.size();

	VK_ASSERT_RESULT(vkCreateDescriptorSetLayout(KVulkanGlobal::device, &descriptorSetLayoutCI, nullptr, &m_Descriptor.layout));

	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &m_Descriptor.layout;

	VK_ASSERT_RESULT(vkCreatePipelineLayout(KVulkanGlobal::device, &pipelineLayoutCI, nullptr, &m_Pipeline.layout));

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = (uint32_t)poolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = frames;
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VK_ASSERT_RESULT(vkCreateDescriptorPool(KVulkanGlobal::device, &descriptorPoolCreateInfo, nullptr, &m_Descriptor.pool));

	for (uint32_t frameIndex = 0; frameIndex < frames; ++frameIndex)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = m_Descriptor.pool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &m_Descriptor.layout;

		VK_ASSERT_RESULT(vkAllocateDescriptorSets(KVulkanGlobal::device, &descriptorSetAllocateInfo, &m_Descriptor.sets[frameIndex]));
	}

	for (uint32_t frameIndex = 0; frameIndex < frames; ++frameIndex)
	{
		for (VkWriteDescriptorSet& write : staticWrites)
		{
			write.dstSet = m_Descriptor.sets[frameIndex];
		}
		vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(staticWrites.size()), staticWrites.data(), 0, nullptr);
	}
}

void KVulkanComputePipeline::CreatePipeline()
{
	KVulkanShader* vulkanShader = (KVulkanShader*)m_ComputeShader.get();
	VkPipelineShaderStageCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderCreateInfo.module = vulkanShader->GetShaderModule();
	shaderCreateInfo.pSpecializationInfo = vulkanShader->GetSpecializationInfo();
	shaderCreateInfo.pName = "main";

	VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	createInfo.layout = m_Pipeline.layout;
	createInfo.stage = shaderCreateInfo;

	vkCreateComputePipelines(KVulkanGlobal::device, KVulkanGlobal::pipelineCache, 1, &createInfo, nullptr, &m_Pipeline.pipeline);
}

void KVulkanComputePipeline::DestroyDescriptorSet()
{
	if (m_Descriptor.pool != VK_NULL_HANDEL)
	{
		vkDestroyDescriptorSetLayout(KVulkanGlobal::device, m_Descriptor.layout, nullptr);
		vkDestroyDescriptorPool(KVulkanGlobal::device, m_Descriptor.pool, nullptr);

		m_Descriptor.layout = VK_NULL_HANDEL;
		m_Descriptor.pool = VK_NULL_HANDEL;
		m_Descriptor.sets.clear();
	}

	if (m_Pipeline.layout != VK_NULL_HANDEL)
	{
		vkDestroyPipelineLayout(KVulkanGlobal::device, m_Pipeline.layout, nullptr);
		m_Pipeline.layout = VK_NULL_HANDEL;
	}
}

void KVulkanComputePipeline::DestroyPipeline()
{
	if (m_Pipeline.pipeline != VK_NULL_HANDEL)
	{
		vkDestroyPipeline(KVulkanGlobal::device, m_Pipeline.pipeline, nullptr);
		m_Pipeline.pipeline = VK_NULL_HANDEL;
	}
}

bool KVulkanComputePipeline::Init(const char* szShader)
{
	UnInit();
	if (KRenderGlobal::ShaderManager.Acquire(ST_COMPUTE, szShader, m_ComputeShader, false))
	{
		CreateDescriptorSet();
		CreatePipeline();
		return true;
	}
	return false;
}

bool KVulkanComputePipeline::UnInit()
{
	DestroyPipeline();
	DestroyDescriptorSet();
	if (m_ComputeShader)
	{
		KRenderGlobal::ShaderManager.Release(m_ComputeShader);
		m_ComputeShader = nullptr;
	}
	return true;
}

bool KVulkanComputePipeline::UpdateDynamicWrite(uint32_t frameIndex, const KDynamicConstantBufferUsage* usage)
{
	if (frameIndex < m_Descriptor.sets.size())
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
				newWrite = PopulateImageWrite(binding, m_Descriptor.sets[frameIndex], location);
			}
			else if (binding.type == BindingInfo::IMAGE)
			{
				newWrite = PopulateImageWrite(binding, m_Descriptor.sets[frameIndex], location);
			}
			else if (binding.type == BindingInfo::AS)
			{
				newWrite = PopulateTopdownASWrite(binding, m_Descriptor.sets[frameIndex], location);
			}
			else if (binding.type == BindingInfo::BUFFER)
			{
				newWrite = PopulateUniformBufferWrite(binding, m_Descriptor.sets[frameIndex], location);
			}
			else if (binding.type == BindingInfo::DYNAMIC_BUFFER)
			{
				ASSERT_RESULT(usage && usage->binding == location);
				newWrite = PopulateDynamicUniformBufferWrite(binding, *usage, m_Descriptor.sets[frameIndex]);
			}
			else
			{
				continue;
			}

			dynamicWrites.push_back(newWrite);
		}

		vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(dynamicWrites.size()), dynamicWrites.data(), 0, nullptr);
		return true;
	}
	return false;
}

bool KVulkanComputePipeline::SetupImageBarrier(IKCommandBufferPtr buffer, bool input)
{
	KVulkanCommandBuffer* commandBuffer = static_cast<KVulkanCommandBuffer*>(buffer.get());
	VkCommandBuffer cmdBuf = commandBuffer->GetVkHandle();

	std::vector<VkImageMemoryBarrier> barriers;
	for (auto& pair : m_Bindings)
	{
		BindingInfo& binding = pair.second;

		if (binding.type == BindingInfo::IMAGE)
		{
			for (size_t i = 0; i < binding.image.images.size(); ++i)
			{
				KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(binding.image.images[i].get());

				VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				VkImageMemoryBarrier imgMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

				if (binding.image.flag == COMPUTE_IMAGE_IN)
				{
					imgMemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				}
				else if (binding.image.flag == COMPUTE_IMAGE_OUT)
				{
					imgMemBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				}

				imgMemBarrier.image = vulkanFrameBuffer->GetImage();
				imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imgMemBarrier.subresourceRange = range;

				barriers.push_back(imgMemBarrier);
			}
		}
	}

	std::vector<IKFrameBufferPtr> translatedFrameBuffers;
	for (auto& pair : m_Bindings)
	{
		BindingInfo& binding = pair.second;

		if (binding.type == BindingInfo::IMAGE && binding.image.flag == COMPUTE_IMAGE_IN)
		{
			for (size_t i = 0; i < binding.image.images.size(); ++i)
			{
				KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(binding.image.images[i].get());
				if (!vulkanFrameBuffer->IsStroageImage())
				{
					translatedFrameBuffers.push_back(binding.image.images[i]);
				}
			}
		}
	}

	if (input)
	{
		for (IKFrameBufferPtr frameBuffer : translatedFrameBuffers)
		{
			buffer->TranslateToStorage(frameBuffer);
		}
		vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
	}
	else
	{
		vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
		for (IKFrameBufferPtr frameBuffer : translatedFrameBuffers)
		{
			buffer->TranslateToShader(frameBuffer);
		}
	}

	return true;
}

bool KVulkanComputePipeline::Execute(IKCommandBufferPtr primaryBuffer, uint32_t groupX, uint32_t groupY, uint32_t groupZ, uint32_t frameIndex, const KDynamicConstantBufferUsage* usage)
{
	if (primaryBuffer && frameIndex < m_Descriptor.sets.size())
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
		UpdateDynamicWrite(frameIndex, usage);

		// Adding a barrier to be sure the fragment has finished writing
		SetupImageBarrier(primaryBuffer, true);

		// Preparing for the compute shader
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline.pipeline);
		vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline.layout, 0, 1, &m_Descriptor.sets[frameIndex], dynamicBufferCount, &dynamicOffsets);
		// Dispatching the shader
		vkCmdDispatch(cmdBuf, groupX, groupY, groupZ);

		// Adding a barrier to be sure the compute shader has finished
		SetupImageBarrier(primaryBuffer, false);

		return true;
	}
	return false;
}

bool KVulkanComputePipeline::ReloadShader()
{
	if (m_ComputeShader && m_ComputeShader->Reload())
	{
		DestroyPipeline();
		CreatePipeline();
		return true;
	}
	return false;
}