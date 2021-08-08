#include "KVulkanComputePipeline.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanAccelerationStructure.h"
#include "KVulkanShader.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"
#include "Internal/KRenderGlobal.h"

KVulkanComputePipeline::KVulkanComputePipeline()
{
}

KVulkanComputePipeline::~KVulkanComputePipeline()
{
}

void KVulkanComputePipeline::SetStorageImage(uint32_t location, IKRenderTargetPtr target, bool dynimicWrite)
{
	BindingInfo newBinding;
	newBinding.target = target;
	newBinding.dynamicWrite = dynimicWrite;
	m_Bindings[location] = newBinding;
}

void KVulkanComputePipeline::SetAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynimicWrite)
{
	BindingInfo newBinding;
	newBinding.as = as;
	newBinding.dynamicWrite = dynimicWrite;
	m_Bindings[location] = newBinding;
}

VkWriteDescriptorSet KVulkanComputePipeline::PopulateImageWrite(IKRenderTargetPtr target, VkDescriptorSet dstSet, uint32_t dstBinding)
{
	VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	KVulkanRenderTarget* vulkanRenderTarget = static_cast<KVulkanRenderTarget*>(target.get());
	KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(vulkanRenderTarget->GetFrameBuffer().get());
	VkDescriptorImageInfo descriptor{ VK_NULL_HANDLE, vulkanFrameBuffer->GetImageView(), VK_IMAGE_LAYOUT_GENERAL };
	return KVulkanInitializer::CreateDescriptorImageWrite(&descriptor, type, dstSet, dstBinding, 1);
}

VkWriteDescriptorSet KVulkanComputePipeline::PopulateTopdownASWrite(IKAccelerationStructurePtr as, VkDescriptorSet dstSet, uint32_t dstBinding)
{
	KVulkanAccelerationStructure* vulkanAccelerationStructure = static_cast<KVulkanAccelerationStructure*>(as.get());
	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &vulkanAccelerationStructure->GetTopDown().handle;
	return KVulkanInitializer::CreateDescriptorAccelerationStructureWrite(&descriptorAccelerationStructureInfo, dstSet, dstBinding, 1);
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

		if (binding.target)
		{
			type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			newWrite = PopulateImageWrite(binding.target, VK_NULL_HANDLE, location);
		}
		else if (binding.as)
		{
			type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			newWrite = PopulateTopdownASWrite(binding.as, VK_NULL_HANDLE, location);
		}
		else
		{
			continue;
		}

		VkDescriptorSetLayoutBinding newBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(type, stageFlags, location);
		VkDescriptorPoolSize newPoolSize = { type, 1};

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
		std::vector<VkWriteDescriptorSet> writes = staticWrites;
		for (VkWriteDescriptorSet& write : writes)
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
		DestroyDescriptorSet();
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

bool KVulkanComputePipeline::UpdateDynamicWrite(uint32_t frameIndex)
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

			if (binding.target)
			{
				newWrite = PopulateImageWrite(binding.target, m_Descriptor.sets[frameIndex], location);
			}
			else if (binding.as)
			{
				newWrite = PopulateTopdownASWrite(binding.as, m_Descriptor.sets[frameIndex], location);
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

bool KVulkanComputePipeline::Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (primaryBuffer && frameIndex < m_Descriptor.sets.size())
	{
		UpdateDynamicWrite(frameIndex);
		return true;
	}
	return false;
}