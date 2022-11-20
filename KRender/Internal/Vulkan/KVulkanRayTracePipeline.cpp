#include "KVulkanRayTracePipeline.h"
#include "KVulkanAccelerationStructure.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanBuffer.h"
#include "KVulkanShader.h"
#include "KVulkanSampler.h"
#include "KVulkanTexture.h"
#include "KVulkanHelper.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Vulkan/KVulkanGlobal.h"
#include "KBase/Publish/KNumerical.h"

KVulkanRayTracePipeline::KVulkanRayTracePipeline()
	: m_CommandPool(nullptr)
	, m_StorageRT(nullptr)
	, m_Format(EF_R8GB8BA8_UNORM)
	, m_Width(0)
	, m_Height(0)
	, m_Inited(false)
	, m_ASUpdated(false)
{
}

KVulkanRayTracePipeline::~KVulkanRayTracePipeline()
{
	ASSERT_RESULT(!m_Inited && "should be destoryed");
}

void KVulkanRayTracePipeline::CreateStorageImage()
{
	ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(m_StorageRT));
	ASSERT_RESULT(m_StorageRT->InitFromStorage(m_Width, m_Height, 1, m_Format));
}

void KVulkanRayTracePipeline::DestroyStorageImage()
{
	SAFE_UNINIT(m_StorageRT);
}

void KVulkanRayTracePipeline::CreateStrogeScene()
{
	KVulkanAccelerationStructure* vulkanAS = (KVulkanAccelerationStructure*)m_TopDownAS.get();
	const std::vector<KVulkanRayTraceInstance>& instances = vulkanAS->GetInstances();
	VkDeviceSize size = (VkDeviceSize)(instances.size() * sizeof(KVulkanRayTraceInstance));
	KVulkanInitializer::CreateStorageBuffer(size, instances.data(), m_Scene.buffer, m_Scene.allocInfo);
}

void KVulkanRayTracePipeline::DestroyStrogeScene()
{
	KVulkanInitializer::DestroyStorageBuffer(m_Scene.buffer, m_Scene.allocInfo);
}

void KVulkanRayTracePipeline::CreateDescriptorSet()
{
	uint32_t frames = KRenderGlobal::NumFramesInFlight;
	m_Descriptor.sets.resize(frames);

	KVulkanAccelerationStructure* topDown = static_cast<KVulkanAccelerationStructure*>(m_TopDownAS.get());
	const std::vector<VkDescriptorImageInfo>& textures = topDown->GetTextureDescriptors();

	uint32_t stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

	VkDescriptorSetLayoutBinding accelerationStructureBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, stageFlags, RAYTRACE_BINDING_AS);
	VkDescriptorSetLayoutBinding storageImageBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, stageFlags, RAYTRACE_BINDING_IMAGE);
	VkDescriptorSetLayoutBinding uniformBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlags, RAYTRACE_BINDING_CAMERA);
	VkDescriptorSetLayoutBinding sceneBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlags, RAYTRACE_BINDING_SCENE);
	VkDescriptorSetLayoutBinding textureBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, RAYTRACE_BINDING_TEXTURES, (uint32_t)std::max(textures.size(), (size_t)1));
	VkDescriptorSetLayoutBinding normalBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, RAYTRACE_BINDING_GBUFFER0);
	VkDescriptorSetLayoutBinding positionBinding = KVulkanInitializer::CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, RAYTRACE_BINDING_GBUFFER1);

	VkDescriptorSetLayoutBinding setLayoutBindings[] =
	{
		accelerationStructureBinding,
		storageImageBinding,
		uniformBinding,
		sceneBinding,
		textureBinding,
		normalBinding,
		positionBinding
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {};
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCI.pBindings = setLayoutBindings;
	descriptorSetLayoutCI.bindingCount = ARRAY_SIZE(setLayoutBindings);

	VK_ASSERT_RESULT(vkCreateDescriptorSetLayout(KVulkanGlobal::device, &descriptorSetLayoutCI, nullptr, &m_Descriptor.layout));

	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &m_Descriptor.layout;

	VK_ASSERT_RESULT(vkCreatePipelineLayout(KVulkanGlobal::device, &pipelineLayoutCI, nullptr, &m_Pipeline.layout));

	VkDescriptorPoolSize poolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = ARRAY_SIZE(poolSizes);
	descriptorPoolCreateInfo.pPoolSizes = poolSizes;
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
		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &topDown->GetTopDown().handle;
		VkWriteDescriptorSet accelerationStructureWrite = KVulkanInitializer::CreateDescriptorAccelerationStructureWrite(&descriptorAccelerationStructureInfo, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_AS);

		KVulkanRenderTarget* vulkanRenderTarget = static_cast<KVulkanRenderTarget*>(m_StorageRT.get());
		KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(vulkanRenderTarget->GetFrameBuffer().get());
		VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, vulkanFrameBuffer->GetImageView(), VK_IMAGE_LAYOUT_GENERAL };	
		VkWriteDescriptorSet storageImageWrite = KVulkanInitializer::CreateDescriptorImageWrite(&storageImageDescriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_IMAGE);

		KVulkanUniformBuffer* vulkanUniformBuffer = static_cast<KVulkanUniformBuffer*>(m_CameraBuffer.get());
		VkDescriptorBufferInfo uniformBufferInfo = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanUniformBuffer->GetVulkanHandle(), 0, vulkanUniformBuffer->GetBufferSize());
		VkWriteDescriptorSet uniformWrite = KVulkanInitializer::CreateDescriptorBufferWrite(&uniformBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_CAMERA);

		VkDescriptorBufferInfo sceneBufferInfo = KVulkanInitializer::CreateDescriptorBufferIntfo(m_Scene.buffer, 0, VK_WHOLE_SIZE);
		VkWriteDescriptorSet sceneWrite = KVulkanInitializer::CreateDescriptorBufferWrite(&sceneBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_SCENE);

		// Keep VK happy
		KSamplerRef errorSampler; ASSERT_RESULT(KRenderGlobal::SamplerManager.GetErrorSampler(errorSampler));
		KTextureRef errorTexture; ASSERT_RESULT(KRenderGlobal::TextureManager.GetErrorTexture(errorTexture));
		VkDescriptorImageInfo emptyImageInfo = KVulkanInitializer::CreateDescriptorImageInfo(
			((KVulkanSampler*)(*errorSampler).get())->GetVkSampler(),
			((KVulkanTexture*)(*errorTexture).get())->GetImageView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
		VkWriteDescriptorSet textureWrite = KVulkanInitializer::CreateDescriptorImageWrite(textures.size() ? textures.data() : &emptyImageInfo,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_TEXTURES, textures.size() ? textureBinding.descriptorCount : 1);

		IKSamplerPtr normalSampler = KRenderGlobal::GBuffer.GetSampler();
		IKRenderTargetPtr normalTarget = KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0);
		VkDescriptorImageInfo normaImageInfo = KVulkanInitializer::CreateDescriptorImageInfo(
			((KVulkanSampler*)normalSampler.get())->GetVkSampler(),
			((KVulkanFrameBuffer*)normalTarget->GetFrameBuffer().get())->GetImageView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
		VkWriteDescriptorSet normalWrite = KVulkanInitializer::CreateDescriptorImageWrite(&normaImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_GBUFFER0);

		IKSamplerPtr positionSampler = KRenderGlobal::GBuffer.GetSampler();
		IKRenderTargetPtr positionTarget = KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0);
		VkDescriptorImageInfo positionImageInfo = KVulkanInitializer::CreateDescriptorImageInfo(
			((KVulkanSampler*)positionSampler.get())->GetVkSampler(),
			((KVulkanFrameBuffer*)positionTarget->GetFrameBuffer().get())->GetImageView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
		VkWriteDescriptorSet positionWrite = KVulkanInitializer::CreateDescriptorImageWrite(&positionImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_GBUFFER1);

		VkWriteDescriptorSet writeDescriptorSets[] =
		{
			// Binding 0: Top level acceleration structure
			accelerationStructureWrite,
			// Binding 1: Ray tracing result image
			storageImageWrite,
			// Binding 2: Uniform data
			uniformWrite,
			// Binding 3: Scene data
			sceneWrite,
			// Binding 4: Texture data
			textureWrite,
			// Binding 5: Normal data
			normalWrite,
			// Binding 6: Position data
			positionWrite
		};

		vkUpdateDescriptorSets(KVulkanGlobal::device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, VK_NULL_HANDLE);
	}
}

void KVulkanRayTracePipeline::DestroyDescriptorSet()
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

void KVulkanRayTracePipeline::CreateShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = static_cast<VkDeviceSize>(KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleSize) * handleCount;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VK_ASSERT_RESULT(vkCreateBuffer(KVulkanGlobal::device, &bufferCreateInfo, nullptr, &shaderBindingTable.buffer));

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(KVulkanGlobal::device, shaderBindingTable.buffer, &memoryRequirements);

	uint32_t memoryTypeIndex = 0;
	ASSERT_RESULT(KVulkanHelper::FindMemoryType(KVulkanGlobal::physicalDevice,
		memoryRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		memoryTypeIndex));

	ASSERT_RESULT(KVulkanHeapAllocator::Alloc(memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferCreateInfo.usage, shaderBindingTable.allocInfo));
	VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, shaderBindingTable.buffer, shaderBindingTable.allocInfo.vkMemroy, shaderBindingTable.allocInfo.vkOffset));

	const uint32_t handleSizeAligned = KNumerical::AlignedSize(KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleSize,
		KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleAlignment);
	VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR = {};
	KVulkanHelper::GetBufferDeviceAddress(shaderBindingTable.buffer, stridedDeviceAddressRegionKHR.deviceAddress);
	stridedDeviceAddressRegionKHR.stride = handleSizeAligned;
	stridedDeviceAddressRegionKHR.size = static_cast<VkDeviceSize>(handleCount) * handleSizeAligned;

	shaderBindingTable.stridedDeviceAddressRegion = stridedDeviceAddressRegionKHR;
}

void KVulkanRayTracePipeline::DestroyShaderBindingTable(ShaderBindingTable& shaderBindingTable)
{
	if (shaderBindingTable.buffer != VK_NULL_HANDEL)
	{
		vkDestroyBuffer(KVulkanGlobal::device, shaderBindingTable.buffer, nullptr);
		KVulkanHeapAllocator::Free(shaderBindingTable.allocInfo);
		shaderBindingTable.mapped = nullptr;
		shaderBindingTable.buffer = VK_NULL_HANDEL;
	}
}

void KVulkanRayTracePipeline::CreateShaderBindingTables()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	m_ShaderGroups.clear();

	VkRayTracingShaderGroupCreateInfoKHR shaderGroup = {};
	shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;

	auto PopulateShaderCreateInfo = [](IKShaderPtr shader, VkShaderStageFlagBits stage)
	{
		KVulkanShader* vulkanShader = (KVulkanShader*)shader.get();
		VkPipelineShaderStageCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.stage = stage;
		shaderCreateInfo.module = vulkanShader->GetShaderModule();
		shaderCreateInfo.pSpecializationInfo = vulkanShader->GetSpecializationInfo();
		shaderCreateInfo.pName = "main";
		return shaderCreateInfo;
	};

	ASSERT_RESULT(m_RayGenShader.shader && "ray gen shader should always exists");
	{
		shaderStages.push_back(PopulateShaderCreateInfo(*m_RayGenShader.shader, VK_SHADER_STAGE_RAYGEN_BIT_KHR));
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		m_ShaderGroups.push_back(shaderGroup);
	}

	if (m_MissShader.shader)
	{
		shaderStages.push_back(PopulateShaderCreateInfo(*m_MissShader.shader, VK_SHADER_STAGE_MISS_BIT_KHR));
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		m_ShaderGroups.push_back(shaderGroup);
	}

	if (m_ClosestHitShader.shader)
	{
		shaderStages.push_back(PopulateShaderCreateInfo(*m_ClosestHitShader.shader, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		m_ShaderGroups.push_back(shaderGroup);
	}

	// TODO m_AnyHitShader

	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI = {};
	rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCI.pStages = shaderStages.data();
	rayTracingPipelineCI.groupCount = static_cast<uint32_t>(m_ShaderGroups.size());
	rayTracingPipelineCI.pGroups = m_ShaderGroups.data();
	rayTracingPipelineCI.maxPipelineRayRecursionDepth = 4;
	rayTracingPipelineCI.layout = m_Pipeline.layout;
	VK_ASSERT_RESULT(KVulkanGlobal::vkCreateRayTracingPipelinesKHR(KVulkanGlobal::device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &m_Pipeline.pipeline));

	const uint32_t handleSize = KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleSize;
	const uint32_t handleSizeAligned = KNumerical::AlignedSize(KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleSize, KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleAlignment);
	const uint32_t groupCount = static_cast<uint32_t>(m_ShaderGroups.size());
	const uint32_t sbtSize = groupCount * handleSizeAligned;

	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	VK_ASSERT_RESULT(KVulkanGlobal::vkGetRayTracingShaderGroupHandlesKHR(KVulkanGlobal::device, m_Pipeline.pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

	CreateShaderBindingTable(m_ShaderBindingTables.raygen, 1);
	CreateShaderBindingTable(m_ShaderBindingTables.miss, 1);
	CreateShaderBindingTable(m_ShaderBindingTables.hit, 1);

	// Copy handles
	uint32_t handleIndex = 0;
	{
		vkMapMemory(KVulkanGlobal::device, m_ShaderBindingTables.raygen.allocInfo.vkMemroy, m_ShaderBindingTables.raygen.allocInfo.vkOffset, VK_WHOLE_SIZE, 0, &m_ShaderBindingTables.raygen.mapped);
		memcpy(m_ShaderBindingTables.raygen.mapped, shaderHandleStorage.data() + (VkDeviceSize)handleSizeAligned * handleIndex, (VkDeviceSize)handleSize);
		vkUnmapMemory(KVulkanGlobal::device, m_ShaderBindingTables.raygen.allocInfo.vkMemroy);
		++handleIndex;
	}
	if (m_MissShader.shader)
	{
		vkMapMemory(KVulkanGlobal::device, m_ShaderBindingTables.miss.allocInfo.vkMemroy, m_ShaderBindingTables.miss.allocInfo.vkOffset, VK_WHOLE_SIZE, 0, &m_ShaderBindingTables.miss.mapped);
		memcpy(m_ShaderBindingTables.miss.mapped, shaderHandleStorage.data() + (VkDeviceSize)handleSizeAligned * handleIndex, handleSize);
		vkUnmapMemory(KVulkanGlobal::device, m_ShaderBindingTables.miss.allocInfo.vkMemroy);
		++handleIndex;
	}
	if (m_ClosestHitShader.shader)
	{
		vkMapMemory(KVulkanGlobal::device, m_ShaderBindingTables.hit.allocInfo.vkMemroy, m_ShaderBindingTables.hit.allocInfo.vkOffset, VK_WHOLE_SIZE, 0, &m_ShaderBindingTables.hit.mapped);
		memcpy(m_ShaderBindingTables.hit.mapped, shaderHandleStorage.data() + (VkDeviceSize)handleSizeAligned * handleIndex, handleSize);
		vkUnmapMemory(KVulkanGlobal::device, m_ShaderBindingTables.hit.allocInfo.vkMemroy);
		++handleIndex;
	}
}

void KVulkanRayTracePipeline::DestroyShaderBindingTables()
{
	DestroyShaderBindingTable(m_ShaderBindingTables.raygen);
	DestroyShaderBindingTable(m_ShaderBindingTables.miss);
	DestroyShaderBindingTable(m_ShaderBindingTables.hit);
	DestroyShaderBindingTable(m_ShaderBindingTables.callable);

	if (m_Pipeline.pipeline != VK_NULL_HANDEL)
	{
		vkDestroyPipeline(KVulkanGlobal::device, m_Pipeline.pipeline, nullptr);
		m_Pipeline.pipeline = VK_NULL_HANDEL;
	}
}

void KVulkanRayTracePipeline::CreateCommandBuffers()
{
	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;
	uint32_t frames = KRenderGlobal::NumFramesInFlight;

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	ASSERT_RESULT(renderDevice->CreateCommandBuffer(m_CommandBuffer));
	ASSERT_RESULT(m_CommandBuffer->Init(m_CommandPool, CBL_SECONDARY));
}

void KVulkanRayTracePipeline::DestroyCommandBuffers()
{
	SAFE_UNINIT(m_CommandBuffer);
	SAFE_UNINIT(m_CommandPool);
}

bool KVulkanRayTracePipeline::SetShaderTable(ShaderType type, const char* szShader)
{
	if (type == ST_ANY_HIT)
	{
		m_AnyHitShader.path = szShader;
	}
	else if (type == ST_CLOSEST_HIT)
	{
		m_ClosestHitShader.path = szShader;
	}
	else if (type == ST_RAYGEN)
	{
		m_RayGenShader.path = szShader;
	}
	else if (type == ST_MISS)
	{
		m_MissShader.path = szShader;
	}
	else
	{
		assert(false && "should not reach");
		return false;
	}
	return true;
}

bool KVulkanRayTracePipeline::SetStorageImage(ElementFormat format)
{
	m_Format = format;
	return true;
}

bool KVulkanRayTracePipeline::RecreateFromAS()
{
	if (m_Inited)
	{
		KRenderGlobal::RenderDevice->Wait();

		DestroyStrogeScene();
		CreateStrogeScene();

		KVulkanAccelerationStructure* topDown = static_cast<KVulkanAccelerationStructure*>(m_TopDownAS.get());

		uint32_t frames = KRenderGlobal::NumFramesInFlight;
		for (uint32_t frameIndex = 0; frameIndex < frames; ++frameIndex)
		{
			VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {};
			descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
			descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
			descriptorAccelerationStructureInfo.pAccelerationStructures = &topDown->GetTopDown().handle;
			VkWriteDescriptorSet accelerationStructureWrite = KVulkanInitializer::CreateDescriptorAccelerationStructureWrite(&descriptorAccelerationStructureInfo, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_AS);

			VkDescriptorBufferInfo sceneBufferInfo = KVulkanInitializer::CreateDescriptorBufferIntfo(m_Scene.buffer, 0, VK_WHOLE_SIZE);
			VkWriteDescriptorSet sceneWrite = KVulkanInitializer::CreateDescriptorBufferWrite(&sceneBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_SCENE);

			VkWriteDescriptorSet writeDescriptorSets[] =
			{
				// Binding 0: Top level acceleration structure
				accelerationStructureWrite,
				// Binding 3: Scene data
				sceneWrite
			};

			vkUpdateDescriptorSets(KVulkanGlobal::device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, VK_NULL_HANDLE);
		}
		return true;
	}
	return false;
}

bool KVulkanRayTracePipeline::ResizeImage(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;

	if (m_Inited)
	{
		KRenderGlobal::RenderDevice->Wait();

		m_StorageRT->UnInit();
		m_StorageRT->InitFromStorage(m_Width, m_Height, 1, m_Format);

		uint32_t frames = KRenderGlobal::NumFramesInFlight;
		for (uint32_t frameIndex = 0; frameIndex < frames; ++frameIndex)
		{
			KVulkanRenderTarget* vulkanRenderTarget = static_cast<KVulkanRenderTarget*>(m_StorageRT.get());
			KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(vulkanRenderTarget->GetFrameBuffer().get());
			VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, vulkanFrameBuffer->GetImageView(), VK_IMAGE_LAYOUT_GENERAL };
			VkWriteDescriptorSet storageImageWrite = KVulkanInitializer::CreateDescriptorImageWrite(&storageImageDescriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_IMAGE);

			IKSamplerPtr normalSampler = KRenderGlobal::GBuffer.GetSampler();
			IKRenderTargetPtr normalTarget = KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0);
			VkDescriptorImageInfo normaImageInfo = KVulkanInitializer::CreateDescriptorImageInfo(
				((KVulkanSampler*)normalSampler.get())->GetVkSampler(),
				((KVulkanFrameBuffer*)normalTarget->GetFrameBuffer().get())->GetImageView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			VkWriteDescriptorSet normalWrite = KVulkanInitializer::CreateDescriptorImageWrite(&normaImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_GBUFFER0);

			IKSamplerPtr positionSampler = KRenderGlobal::GBuffer.GetSampler();
			IKRenderTargetPtr positionTarget = KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0);
			VkDescriptorImageInfo positionImageInfo = KVulkanInitializer::CreateDescriptorImageInfo(
				((KVulkanSampler*)positionSampler.get())->GetVkSampler(),
				((KVulkanFrameBuffer*)positionTarget->GetFrameBuffer().get())->GetImageView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			VkWriteDescriptorSet positionWrite = KVulkanInitializer::CreateDescriptorImageWrite(&positionImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Descriptor.sets[frameIndex], RAYTRACE_BINDING_GBUFFER1);

			VkWriteDescriptorSet writeDescriptorSets[] =
			{
				// Binding 1: Ray tracing result image
				storageImageWrite,
				// Binding 5: Normal data
				normalWrite,
				// Binding 6: Position data
				positionWrite
			};

			vkUpdateDescriptorSets(KVulkanGlobal::device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, VK_NULL_HANDLE);
		}

		return true;
	}
	return false;
}

void KVulkanRayTracePipeline::CreateShader()
{
	auto AcquireShader = [](ShaderInfo& shaderInfo, ShaderType type)
	{
		if (!shaderInfo.path.empty())
		{
			ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(type, shaderInfo.path.c_str(), shaderInfo.shader, false));
		}
	};
	AcquireShader(m_AnyHitShader, ST_ANY_HIT);
	AcquireShader(m_ClosestHitShader, ST_CLOSEST_HIT);
	AcquireShader(m_RayGenShader, ST_RAYGEN);
	AcquireShader(m_MissShader, ST_MISS);
}

void KVulkanRayTracePipeline::DestroyShader()
{
	m_AnyHitShader.shader.Release();
	m_ClosestHitShader.shader.Release();
	m_RayGenShader.shader.Release();
	m_MissShader.shader.Release();
}

bool KVulkanRayTracePipeline::ReloadShader()
{
	if (m_AnyHitShader.shader|| m_ClosestHitShader.shader || m_RayGenShader.shader || m_MissShader.shader)
	{
		if (m_AnyHitShader.shader) (*m_AnyHitShader.shader)->Reload();
		if (m_ClosestHitShader.shader) (*m_ClosestHitShader.shader)->Reload();
		if (m_RayGenShader.shader) (*m_RayGenShader.shader)->Reload();
		if (m_MissShader.shader) (*m_MissShader.shader)->Reload();

		DestroyShaderBindingTables();
		CreateShaderBindingTables();

		return true;
	}
	return false;
}

IKRenderTargetPtr KVulkanRayTracePipeline::GetStorageTarget()
{
	return m_StorageRT;
}

bool KVulkanRayTracePipeline::Init(IKUniformBufferPtr cameraBuffer, IKAccelerationStructurePtr topDownAS, uint32_t width, uint32_t height)
{
	UnInit();

	m_CameraBuffer = cameraBuffer;
	m_TopDownAS = topDownAS;
	m_Width = width;
	m_Height = height;

	CreateStorageImage();

	if (KVulkanGlobal::supportRaytrace)
	{
		CreateStrogeScene();
		CreateDescriptorSet();
		CreateShader();
		CreateShaderBindingTables();
		CreateCommandBuffers();
	}

	m_Inited = true;
	m_ASUpdated = false;
	return true;
}

bool KVulkanRayTracePipeline::UnInit()
{
	DestroyStorageImage();

	if (KVulkanGlobal::supportRaytrace)
	{
		DestroyStrogeScene();
		DestroyShaderBindingTables();
		DestroyDescriptorSet();
		DestroyCommandBuffers();
		DestroyShader();
	}

	m_CameraBuffer = nullptr;
	m_TopDownAS = nullptr;

	m_Inited = false;
	return true;
}

bool KVulkanRayTracePipeline::MarkASUpdated()
{
	m_ASUpdated = true;
	return true;
}

void KVulkanRayTracePipeline::UpdateCameraDescriptor()
{
	KVulkanUniformBuffer* vulkanUniformBuffer = static_cast<KVulkanUniformBuffer*>(m_CameraBuffer.get());
	VkDescriptorBufferInfo uniformBufferInfo = KVulkanInitializer::CreateDescriptorBufferIntfo(vulkanUniformBuffer->GetVulkanHandle(), 0, vulkanUniformBuffer->GetBufferSize());
	VkWriteDescriptorSet uniformWrite = KVulkanInitializer::CreateDescriptorBufferWrite(&uniformBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_Descriptor.sets[KRenderGlobal::CurrentFrameIndex], RAYTRACE_BINDING_CAMERA);

	VkWriteDescriptorSet writeDescriptorSets[] =
	{
		// Binding 2: Uniform data
		uniformWrite
	};

	vkUpdateDescriptorSets(KVulkanGlobal::device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, VK_NULL_HANDLE);
}

bool KVulkanRayTracePipeline::Execute(IKCommandBufferPtr primaryBuffer)
{
	uint32_t frameIndex = KRenderGlobal::CurrentFrameIndex;
	if (KVulkanGlobal::supportRaytrace)
	{
		if (primaryBuffer && frameIndex < m_Descriptor.sets.size())
		{
			if (m_ASUpdated)
			{
				m_ASUpdated = false;
				RecreateFromAS();
			}

			UpdateCameraDescriptor();

			IKCommandBufferPtr commandBuffer = primaryBuffer;
			KVulkanCommandBuffer* vulkanCommandBuffer = (KVulkanCommandBuffer*)(commandBuffer.get());
			VkCommandBuffer vkCommandBuffer = vulkanCommandBuffer->GetVkHandle();

			vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Pipeline.pipeline);
			vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Pipeline.layout, 0, 1, &m_Descriptor.sets[frameIndex], 0, 0);

			VkStridedDeviceAddressRegionKHR emptySbtEntry = {};
			KVulkanGlobal::vkCmdTraceRaysKHR(
				vkCommandBuffer,
				&m_ShaderBindingTables.raygen.stridedDeviceAddressRegion,
				&m_ShaderBindingTables.miss.stridedDeviceAddressRegion,
				&m_ShaderBindingTables.hit.stridedDeviceAddressRegion,
				&emptySbtEntry,
				m_Width,
				m_Height,
				1);

			return true;
		}
	}
	return false;
}