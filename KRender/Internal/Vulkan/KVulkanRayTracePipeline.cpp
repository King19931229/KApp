#include "KVulkanRayTracePipeline.h"
#include "KVulkanAccelerationStructure.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanShader.h"
#include "KVulkanHelper.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Vulkan/KVulkanGlobal.h"
#include "KBase/Publish/KNumerical.h"

KVulkanRayTracePipeline::KVulkanRayTracePipeline()
	: m_StorgeRT(nullptr)
	, m_AnyHitShader(nullptr)
	, m_ClosestHitShader(nullptr)
	, m_RayGenShader(nullptr)
	, m_MissShader(nullptr)
	, m_Format(EF_R8GB8BA8_UNORM)
	, m_Width(0)
	, m_Height(0)
	, m_Inited(false)
{
}

KVulkanRayTracePipeline::~KVulkanRayTracePipeline()
{
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

void KVulkanRayTracePipeline::CreateStorgeImage()
{
	ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(m_StorgeRT));
	ASSERT_RESULT(m_StorgeRT->InitFromStroge(m_Width, m_Height, m_Format));
}

void KVulkanRayTracePipeline::DestroyStorgeImage()
{
	SAFE_UNINIT(m_StorgeRT);
}

void KVulkanRayTracePipeline::CreateDescriptorSet()
{
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

	VkDescriptorSetLayoutBinding setLayoutBindings[] =
	{
		accelerationStructureBinding,
		storageImageBinding,
		// uniformBinding
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
		//{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = ARRAY_SIZE(poolSizes);
	descriptorPoolCreateInfo.pPoolSizes = poolSizes;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VK_ASSERT_RESULT(vkCreateDescriptorPool(KVulkanGlobal::device, &descriptorPoolCreateInfo, nullptr, &m_Descriptor.pool));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_Descriptor.pool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &m_Descriptor.layout;

	VK_ASSERT_RESULT(vkAllocateDescriptorSets(KVulkanGlobal::device, &descriptorSetAllocateInfo, &m_Descriptor.set));

	KVulkanAccelerationStructure* topDown = static_cast<KVulkanAccelerationStructure*>(m_TopDown.get());

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &topDown->GetTopDown().handle;

	VkWriteDescriptorSet accelerationStructureWrite = {};
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = m_Descriptor.set;
	accelerationStructureWrite.dstBinding = 0;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	KVulkanRenderTarget* vulkanRenderTarget = static_cast<KVulkanRenderTarget*>(m_StorgeRT.get());
	KVulkanFrameBuffer* vulkanFrameBuffer = static_cast<KVulkanFrameBuffer*>(vulkanRenderTarget->GetFrameBuffer().get());
	VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, vulkanFrameBuffer->GetImageView(), VK_IMAGE_LAYOUT_GENERAL };

	VkWriteDescriptorSet storageImageWrite = {};
	storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	storageImageWrite.dstSet = m_Descriptor.set;
	storageImageWrite.dstBinding = 1;
	storageImageWrite.descriptorCount = 1;
	storageImageWrite.pImageInfo = &storageImageDescriptor;
	storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

	VkWriteDescriptorSet uniformWrite = {};
	uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniformWrite.dstSet = m_Descriptor.set;
	uniformWrite.dstBinding = 2;
	uniformWrite.descriptorCount = 1;
	uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	VkWriteDescriptorSet writeDescriptorSets[] =
	{
		// Binding 0: Top level acceleration structure
		accelerationStructureWrite,
		// Binding 1: Ray tracing result image
		storageImageWrite,
		// Binding 2: Uniform data
		// uniformWrite,
	};

	vkUpdateDescriptorSets(KVulkanGlobal::device, ARRAY_SIZE(writeDescriptorSets), writeDescriptorSets, 0, VK_NULL_HANDLE);
}

void KVulkanRayTracePipeline::DestroyDescriptorSet()
{
	if (m_Descriptor.pool != VK_NULL_HANDEL)
	{
		vkDestroyDescriptorSetLayout(KVulkanGlobal::device, m_Descriptor.layout, nullptr);
		vkDestroyDescriptorPool(KVulkanGlobal::device, m_Descriptor.pool, nullptr);

		m_Descriptor.layout = VK_NULL_HANDEL;
		m_Descriptor.set = VK_NULL_HANDEL;
		m_Descriptor.pool = VK_NULL_HANDEL;
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

	ASSERT_RESULT(KVulkanHeapAllocator::Alloc(memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderBindingTable.allocInfo));
	VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, shaderBindingTable.buffer, shaderBindingTable.allocInfo.vkMemroy, shaderBindingTable.allocInfo.vkOffset));

	const uint32_t handleSizeAligned = KNumerical::AlignedSize(KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleSize,
		KVulkanGlobal::rayTracingPipelineProperties.shaderGroupHandleAlignment);
	VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR = {};
	KVulkanHelper::GetBufferDeviceAddress(shaderBindingTable.buffer, stridedDeviceAddressRegionKHR.deviceAddress);
	stridedDeviceAddressRegionKHR.stride = handleSizeAligned;
	stridedDeviceAddressRegionKHR.size = static_cast<VkDeviceSize>(handleCount) * handleSizeAligned;

	shaderBindingTable.stridedDeviceAddressRegion = stridedDeviceAddressRegionKHR;

	vkMapMemory(KVulkanGlobal::device, shaderBindingTable.allocInfo.vkMemroy, shaderBindingTable.allocInfo.vkOffset, VK_WHOLE_SIZE, 0, &shaderBindingTable.mapped);
}

void KVulkanRayTracePipeline::DestroyShaderBindingTable(ShaderBindingTable& shaderBindingTable)
{
	if (shaderBindingTable.buffer != VK_NULL_HANDEL)
	{
		vkDestroyBuffer(KVulkanGlobal::device, shaderBindingTable.buffer, nullptr);
		KVulkanHeapAllocator::Free(shaderBindingTable.allocInfo);
		shaderBindingTable.mapped = nullptr;
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
		shaderStages.push_back(PopulateShaderCreateInfo(m_ClosestHitShader, VK_SHADER_STAGE_MISS_BIT_KHR));
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
		memcpy(m_ShaderBindingTables.raygen.mapped, shaderHandleStorage.data() + (VkDeviceSize)handleSizeAligned * handleIndex, (VkDeviceSize)handleSize);
		++handleIndex;
	}
	if (m_MissShader)
	{
		memcpy(m_ShaderBindingTables.miss.mapped, shaderHandleStorage.data() + (VkDeviceSize)handleSizeAligned * handleIndex, handleSize);
		++handleIndex;
	}
	if (m_ClosestHitShader)
	{
		memcpy(m_ShaderBindingTables.hit.mapped, shaderHandleStorage.data() + (VkDeviceSize)handleSizeAligned * handleIndex, handleSize);
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

bool KVulkanRayTracePipeline::SetStorgeImage(ElementFormat format, uint32_t width, uint32_t height)
{
	m_Format = format;
	m_Width = width;
	m_Height = height;
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
		DestroyShaderBindingTables();
		DestroyDescriptorSet();
		DestroyAccelerationStructure();

		CreateAccelerationStructure();
		CreateDescriptorSet();
		CreateShaderBindingTables();
		return true;
	}
	return false;
}

bool KVulkanRayTracePipeline::ResizeImage(uint32_t width, uint32_t height)
{
	if (m_Inited)
	{
		DestroyStorgeImage();
		DestroyDescriptorSet();
		DestroyAccelerationStructure();

		CreateStorgeImage();
		CreateDescriptorSet();
		CreateShaderBindingTables();
		return true;
	}
	return false;
}

bool KVulkanRayTracePipeline::Init()
{
	UnInit();

	CreateStorgeImage();
	CreateAccelerationStructure();
	CreateDescriptorSet();
	CreateShaderBindingTables();

	m_Inited = true;
	return true;
}

bool KVulkanRayTracePipeline::UnInit()
{
	DestroyStorgeImage();
	DestroyAccelerationStructure();
	DestroyShaderBindingTables();
	DestroyDescriptorSet();

	m_Inited = false;
	return true;
}