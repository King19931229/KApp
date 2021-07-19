#include "KVulkanRayTracePipeline.h"
#include "KVulkanAccelerationStructure.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanBuffer.h"
#include "KVulkanShader.h"
#include "KVulkanHelper.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Vulkan/KVulkanGlobal.h"
#include "KBase/Publish/KNumerical.h"

KVulkanRayTracePipeline::KVulkanRayTracePipeline()
	: m_TopDown(nullptr)
	, m_CommandPool(nullptr)
	, m_StorageRT(nullptr)
	, m_AnyHitShader(nullptr)
	, m_ClosestHitShader(nullptr)
	, m_RayGenShader(nullptr)
	, m_MissShader(nullptr)
	, m_Format(EF_R8GB8BA8_UNORM)
	, m_Width(0)
	, m_Height(0)
	, m_Inited(false)
	, m_ASNeedUpdate(false)
{
}

KVulkanRayTracePipeline::~KVulkanRayTracePipeline()
{
	// 暂时先认为Shader不是通过Manager创建的
	SAFE_UNINIT(m_AnyHitShader);
	SAFE_UNINIT(m_ClosestHitShader);
	SAFE_UNINIT(m_RayGenShader);
	SAFE_UNINIT(m_MissShader);
	ASSERT_RESULT(!m_Inited && "should be destoryed");
}

void KVulkanRayTracePipeline::CreateAccelerationStructure()
{
	ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateAccelerationStructure(m_TopDown));
	std::vector<IKAccelerationStructure::BottomASTransformTuple> bottomASs;
	bottomASs.reserve(m_BottomASMap.size());
	for (auto it = m_BottomASMap.begin(), itEnd = m_BottomASMap.end(); it != itEnd; ++it)
	{
		bottomASs.push_back(it->second);
	}
	ASSERT_RESULT(m_TopDown->InitTopDown(bottomASs));
}

void KVulkanRayTracePipeline::DestroyAccelerationStructure()
{
	SAFE_UNINIT(m_TopDown);
}

void KVulkanRayTracePipeline::CreateStorageImage()
{
	ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(m_StorageRT));
	ASSERT_RESULT(m_StorageRT->InitFromStroge(m_Width, m_Height, m_Format));
}

void KVulkanRayTracePipeline::DestroyStorageImage()
{
	SAFE_UNINIT(m_StorageRT);
}

void KVulkanRayTracePipeline::CreateStrogeScene()
{
	KVulkanAccelerationStructure* vulkanAS = (KVulkanAccelerationStructure*)m_TopDown.get();
	const std::vector<KVulkanRayTraceInstance>& instances = vulkanAS->GetInstances();
	VkDeviceSize size = (VkDeviceSize)(instances.size() * sizeof(KVulkanRayTraceInstance));
	KVulkanInitializer::CreateStroageBuffer(size, instances.data(), m_Scene.buffer, m_Scene.allocInfo);
}

void KVulkanRayTracePipeline::DestroyStrogeScene()
{
	KVulkanInitializer::DestroyStroageBuffer(m_Scene.buffer, m_Scene.allocInfo);
}

void KVulkanRayTracePipeline::CreateDescriptorSet()
{
	uint32_t frames = KRenderGlobal::RenderDevice->GetNumFramesInFlight();
	m_Descriptor.sets.resize(frames);

	ASSERT_RESULT(m_CameraBuffers.size() == frames);

	VkDescriptorSetLayoutBinding accelerationStructureBinding = {};
	accelerationStructureBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	accelerationStructureBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR; // TODO
	accelerationStructureBinding.binding = 0;
	accelerationStructureBinding.descriptorCount = 1;

	VkDescriptorSetLayoutBinding storageImageBinding = {};
	storageImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	storageImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	storageImageBinding.binding = 1;
	storageImageBinding.descriptorCount = 1;

	VkDescriptorSetLayoutBinding uniformBinding = {};
	uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
	uniformBinding.binding = 2;
	uniformBinding.descriptorCount = 1;

	VkDescriptorSetLayoutBinding sceneBinding = {};
	sceneBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	sceneBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
	sceneBinding.binding = 3;
	sceneBinding.descriptorCount = 1;

	VkDescriptorSetLayoutBinding setLayoutBindings[] =
	{
		accelerationStructureBinding,
		storageImageBinding,
		uniformBinding,
		sceneBinding
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
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
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

	KVulkanAccelerationStructure* topDown = static_cast<KVulkanAccelerationStructure*>(m_TopDown.get());

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &topDown->GetTopDown().handle;

	for (uint32_t frameIndex = 0; frameIndex < frames; ++frameIndex)
	{
		VkWriteDescriptorSet accelerationStructureWrite = {};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// The specialized acceleration structure descriptor has to be chained
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = m_Descriptor.sets[frameIndex];
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		KVulkanRenderTarget* vulkanRenderTarget = static_cast<KVulkanRenderTarget*>(m_StorageRT.get());
		KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(vulkanRenderTarget->GetFrameBuffer().get());

		VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, vulkanFrameBuffer->GetImageView(), VK_IMAGE_LAYOUT_GENERAL };	
		VkWriteDescriptorSet storageImageWrite = {};
		storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		storageImageWrite.dstSet = m_Descriptor.sets[frameIndex];
		storageImageWrite.dstBinding = 1;
		storageImageWrite.descriptorCount = 1;
		storageImageWrite.pImageInfo = &storageImageDescriptor;
		storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		KVulkanUniformBuffer* vulkanUniformBuffer = static_cast<KVulkanUniformBuffer*>(m_CameraBuffers[frameIndex].get());

		VkDescriptorBufferInfo uniformBufferInfo = {};
		uniformBufferInfo.buffer = vulkanUniformBuffer->GetVulkanHandle();
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = vulkanUniformBuffer->GetBufferSize();

		VkWriteDescriptorSet uniformWrite = {};
		uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformWrite.dstSet = m_Descriptor.sets[frameIndex];
		uniformWrite.dstBinding = 2;
		uniformWrite.descriptorCount = 1;
		uniformWrite.pBufferInfo = &uniformBufferInfo;
		uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorBufferInfo sceneBufferInfo;
		sceneBufferInfo.buffer = m_Scene.buffer;
		sceneBufferInfo.offset = 0;
		sceneBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet sceneWrite = {};
		sceneWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		sceneWrite.dstSet = m_Descriptor.sets[frameIndex];
		sceneWrite.dstBinding = 3;
		sceneWrite.descriptorCount = 1;
		sceneWrite.pBufferInfo = &sceneBufferInfo;
		sceneWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		VkWriteDescriptorSet writeDescriptorSets[] =
		{
			// Binding 0: Top level acceleration structure
			accelerationStructureWrite,
			// Binding 1: Ray tracing result image
			storageImageWrite,
			// Binding 2: Uniform data
			uniformWrite,
			// Binding 3: Scene data
			sceneWrite
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

	ASSERT_RESULT(m_RayGenShader && "ray gen shader should always exists");
	{
		shaderStages.push_back(PopulateShaderCreateInfo(m_RayGenShader, VK_SHADER_STAGE_RAYGEN_BIT_KHR));
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		m_ShaderGroups.push_back(shaderGroup);
	}

	if (m_MissShader)
	{
		shaderStages.push_back(PopulateShaderCreateInfo(m_MissShader, VK_SHADER_STAGE_MISS_BIT_KHR));
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		m_ShaderGroups.push_back(shaderGroup);
	}

	if (m_ClosestHitShader)
	{
		shaderStages.push_back(PopulateShaderCreateInfo(m_ClosestHitShader, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
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
	VK_ASSERT_RESULT(KVulkanGlobal::vkCreateRayTracingPipelinesKHR(KVulkanGlobal::device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &m_Pipeline.pipline));

	const uint32_t handleSize = KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleSize;
	const uint32_t handleSizeAligned = KNumerical::AlignedSize(KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleSize, KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleAlignment);
	const uint32_t groupCount = static_cast<uint32_t>(m_ShaderGroups.size());
	const uint32_t sbtSize = groupCount * handleSizeAligned;

	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	VK_ASSERT_RESULT(KVulkanGlobal::vkGetRayTracingShaderGroupHandlesKHR(KVulkanGlobal::device, m_Pipeline.pipline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

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
	if (m_MissShader)
	{
		vkMapMemory(KVulkanGlobal::device, m_ShaderBindingTables.miss.allocInfo.vkMemroy, m_ShaderBindingTables.miss.allocInfo.vkOffset, VK_WHOLE_SIZE, 0, &m_ShaderBindingTables.miss.mapped);
		memcpy(m_ShaderBindingTables.miss.mapped, shaderHandleStorage.data() + (VkDeviceSize)handleSizeAligned * handleIndex, handleSize);
		vkUnmapMemory(KVulkanGlobal::device, m_ShaderBindingTables.miss.allocInfo.vkMemroy);
		++handleIndex;
	}
	if (m_ClosestHitShader)
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

	if (m_Pipeline.layout != VK_NULL_HANDEL)
	{
		vkDestroyPipelineLayout(KVulkanGlobal::device, m_Pipeline.layout, nullptr);
		vkDestroyPipeline(KVulkanGlobal::device, m_Pipeline.pipline, nullptr);

		m_Pipeline.layout = VK_NULL_HANDEL;
		m_Pipeline.pipline = VK_NULL_HANDEL;
	}
}

void KVulkanRayTracePipeline::CreateCommandBuffers()
{
	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;
	uint32_t frames = renderDevice->GetNumFramesInFlight();

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);
	m_CommandBuffers.resize(frames);

	for (size_t i = 0; i < frames; ++i)
	{
		IKCommandBufferPtr& buffer = m_CommandBuffers[i];
		ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
		ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
	}
}

void KVulkanRayTracePipeline::DestroyCommandBuffers()
{
	SAFE_UNINIT_CONTAINER(m_CommandBuffers);
	SAFE_UNINIT(m_CommandPool);
}

bool KVulkanRayTracePipeline::SetShaderTable(ShaderType type, IKShaderPtr shader)
{
	if (type == ST_ANY_HIT)
	{
		m_AnyHitShader = shader;
		return true;
	}
	else if (type == ST_CLOSEST_HIT)
	{
		m_ClosestHitShader = shader;
		return true;
	}
	else if (type == ST_RAYGEN)
	{
		m_RayGenShader = shader;
		return true;
	}
	else if (type == ST_MISS)
	{
		m_MissShader = shader;
		return true;
	}
	else
	{
		assert(false && "should not reach");
		return false;
	}
}

bool KVulkanRayTracePipeline::SetStorageImage(ElementFormat format)
{
	m_Format = format;
	return true;
}

uint32_t KVulkanRayTracePipeline::AddBottomLevelAS(IKAccelerationStructurePtr as, const glm::mat4& transform)
{
	uint32_t handle = m_Handles.NewHandle();
	m_BottomASMap[handle] = std::make_tuple(as, transform);
	return handle;
}

bool KVulkanRayTracePipeline::RemoveBottomLevelAS(uint32_t handle)
{
	auto it = m_BottomASMap.find(handle);
	if (it != m_BottomASMap.end())
	{
		m_BottomASMap.erase(it);
		m_Handles.ReleaseHandle(handle);
	}
	return true;
}

bool KVulkanRayTracePipeline::ClearBottomLevelAS()
{
	m_BottomASMap.clear();
	m_Handles.Clear();
	return true;
}

bool KVulkanRayTracePipeline::RecreateAS()
{
	if (m_Inited)
	{
		KRenderGlobal::RenderDevice->Wait();

		DestroyAccelerationStructure();
		CreateAccelerationStructure();
		DestroyStrogeScene();
		CreateStrogeScene();

		KVulkanAccelerationStructure* topDown = static_cast<KVulkanAccelerationStructure*>(m_TopDown.get());

		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &topDown->GetTopDown().handle;

		uint32_t frames = KRenderGlobal::RenderDevice->GetNumFramesInFlight();
		for (uint32_t frameIndex = 0; frameIndex < frames; ++frameIndex)
		{
			VkWriteDescriptorSet accelerationStructureWrite = {};
			accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			// The specialized acceleration structure descriptor has to be chained
			accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
			accelerationStructureWrite.dstSet = m_Descriptor.sets[frameIndex];
			accelerationStructureWrite.dstBinding = 0;
			accelerationStructureWrite.descriptorCount = 1;
			accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

			VkDescriptorBufferInfo sceneBufferInfo;
			sceneBufferInfo.buffer = m_Scene.buffer;
			sceneBufferInfo.offset = 0;
			sceneBufferInfo.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet sceneWrite = {};
			sceneWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			sceneWrite.dstSet = m_Descriptor.sets[frameIndex];
			sceneWrite.dstBinding = 3;
			sceneWrite.descriptorCount = 1;
			sceneWrite.pBufferInfo = &sceneBufferInfo;
			sceneWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

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
		m_StorageRT->InitFromStroge(m_Width, m_Height, m_Format);

		uint32_t frames = KRenderGlobal::RenderDevice->GetNumFramesInFlight();
		for (uint32_t frameIndex = 0; frameIndex < frames; ++frameIndex)
		{
			KVulkanRenderTarget* vulkanRenderTarget = static_cast<KVulkanRenderTarget*>(m_StorageRT.get());
			KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(vulkanRenderTarget->GetFrameBuffer().get());

			VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, vulkanFrameBuffer->GetImageView(), VK_IMAGE_LAYOUT_GENERAL };
			VkWriteDescriptorSet storageImageWrite = {};
			storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			storageImageWrite.dstSet = m_Descriptor.sets[frameIndex];
			storageImageWrite.dstBinding = 1;
			storageImageWrite.descriptorCount = 1;
			storageImageWrite.pImageInfo = &storageImageDescriptor;
			storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

			VkWriteDescriptorSet writeDescriptorSets[] =
			{
				// Binding 1: Ray tracing result image
				storageImageWrite
			};

			vkUpdateDescriptorSets(KVulkanGlobal::device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, VK_NULL_HANDLE);
		}

		return true;
	}
	return false;
}

IKRenderTargetPtr KVulkanRayTracePipeline::GetStorageTarget()
{
	return m_StorageRT;
}

bool KVulkanRayTracePipeline::Init(const std::vector<IKUniformBufferPtr>& cameraBuffers, uint32_t width, uint32_t height)
{
	UnInit();

	m_CameraBuffers = cameraBuffers;
	m_Width = width;
	m_Height = height;

	CreateStorageImage();
	CreateAccelerationStructure();
	CreateStrogeScene();
	CreateDescriptorSet();
	CreateShaderBindingTables();
	CreateCommandBuffers();

	m_Inited = true;
	m_ASNeedUpdate = false;
	return true;
}

bool KVulkanRayTracePipeline::UnInit()
{
	DestroyStorageImage();
	DestroyAccelerationStructure();
	DestroyStrogeScene();
	DestroyShaderBindingTables();
	DestroyDescriptorSet();
	DestroyCommandBuffers();

	m_CameraBuffers.clear();

	m_Inited = false;
	return true;
}

bool KVulkanRayTracePipeline::MarkASNeedUpdate()
{
	m_ASNeedUpdate = true;
	return true;
}

bool KVulkanRayTracePipeline::Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (primaryBuffer && frameIndex < m_Descriptor.sets.size())
	{
		if (m_ASNeedUpdate)
		{
			m_ASNeedUpdate = false;
			RecreateAS();
		}

		IKCommandBufferPtr commandBuffer = primaryBuffer;
		KVulkanCommandBuffer* vulkanCommandBuffer = (KVulkanCommandBuffer*)(commandBuffer.get());
		VkCommandBuffer vkCommandBuffer = vulkanCommandBuffer->GetVkHandle();

		vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Pipeline.pipline);
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

	return false;
}