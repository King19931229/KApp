#include "KVulkanAccelerationStructure.h"
#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

KVulkanAccelerationStructure::KVulkanAccelerationStructure()
	: m_IndexBuffer(nullptr)
	, m_VertexBuffer(nullptr)
{
}

KVulkanAccelerationStructure::~KVulkanAccelerationStructure()
{
	ASSERT_RESULT(!m_IndexBuffer);
	ASSERT_RESULT(!m_VertexBuffer);
}

bool KVulkanAccelerationStructure::InitBottomUp(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer)
{
	if (format != VF_UNKNOWN && vertexBuffer && indexBuffer)
	{
		KVulkanVertexBuffer* vulkanVertexBuffer = static_cast<KVulkanVertexBuffer*>(vertexBuffer.get());
		KVulkanIndexBuffer* vulkanIndexBuffer = static_cast<KVulkanIndexBuffer*>(indexBuffer.get());

		m_IndexBuffer = vulkanIndexBuffer;
		m_VertexBuffer = vulkanVertexBuffer;

		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress = {};
		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress = {};

		ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(vulkanVertexBuffer->GetVulkanHandle(), vertexBufferDeviceAddress.deviceAddress));
		ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(vulkanIndexBuffer->GetVulkanHandle(), indexBufferDeviceAddress.deviceAddress));

		// Always triangle list for now 
		uint32_t numTriangles = (uint32_t)vulkanIndexBuffer->GetIndexCount() / 3;

		// Build
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.maxVertex = (uint32_t)vulkanVertexBuffer->GetVertexCount();
		accelerationStructureGeometry.geometry.triangles.vertexStride = (uint32_t)vulkanVertexBuffer->GetVertexSize();
		accelerationStructureGeometry.geometry.triangles.indexType = vulkanIndexBuffer->GetIndexType() == IT_32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
		accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
		accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;

		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		KVulkanGlobal::vkGetAccelerationStructureBuildSizesKHR(
			KVulkanGlobal::device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&numTriangles,
			&accelerationStructureBuildSizesInfo);

		KVulkanInitializer::CreateVkAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, accelerationStructureBuildSizesInfo, m_BottomUpAS);
		KVulkanInitializer::BuildBottomUpVkAccelerationStructure(accelerationStructureGeometry, accelerationStructureBuildSizesInfo, numTriangles, m_BottomUpAS);

		return true;
	}

	return false;
}

bool KVulkanAccelerationStructure::InitTopDown(const std::vector<BottomASTransformTuple>& bottomASs)
{
	m_Instances.clear();
	m_Instances.reserve(bottomASs.size());

	std::vector<VkAccelerationStructureInstanceKHR> instances;
	instances.reserve(bottomASs.size());

	for (size_t index = 0; index < bottomASs.size(); ++index)
	{
		const BottomASTransformTuple& asTuple = bottomASs[index];
		IKAccelerationStructurePtr as = std::get<0>(asTuple);
		const glm::mat4& transform = std::get<1>(asTuple);

		// glm matrix column major
		VkTransformMatrixKHR transformMatrix =
		{
			transform[0][0], transform[1][0], transform[2][0], transform[3][0],
			transform[0][1], transform[1][1], transform[2][1], transform[3][1],
			transform[0][2], transform[1][2], transform[2][2], transform[3][2]
		};

		KVulkanAccelerationStructure* vulkanAS = static_cast<KVulkanAccelerationStructure*>(as.get());

		VkAccelerationStructureInstanceKHR instance = {};
		instance.transform = transformMatrix;
		instance.instanceCustomIndex = (uint32_t)index;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = vulkanAS->GetBottomUp().deviceAddress;

		instances.push_back(instance);

		KVulkanRayTraceInstance rayInstance = {};
		rayInstance.transform = transform;
		rayInstance.transformIT = glm::inverse(glm::transpose(transform));
		rayInstance.objIndex = (uint32_t)index;
		rayInstance.placeholder = 0;
		rayInstance.vertices = vulkanAS->GetVertexBuffer()->GetDeviceAddress();
		rayInstance.indices = vulkanAS->GetIndexBuffer()->GetDeviceAddress();
		m_Instances.push_back(rayInstance);
	}

	// Buffer for instance data
	VkBuffer instanceBufferHandle = VK_NULL_HANDEL;
	KVulkanHeapAllocator::AllocInfo instanceAlloc;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
	// 以防instances为空
	bufferCreateInfo.size = std::max((size_t)bufferCreateInfo.size, sizeof(VkAccelerationStructureInstanceKHR));
	bufferCreateInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	VK_ASSERT_RESULT(vkCreateBuffer(KVulkanGlobal::device, &bufferCreateInfo, nullptr, &instanceBufferHandle));

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(KVulkanGlobal::device, instanceBufferHandle, &memoryRequirements);

	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	uint32_t memoryTypeIndex = 0;
	ASSERT_RESULT(KVulkanHelper::FindMemoryType(KVulkanGlobal::physicalDevice,
		memoryRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		memoryTypeIndex));

	ASSERT_RESULT(KVulkanHeapAllocator::Alloc(memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, instanceAlloc));
	VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, instanceBufferHandle, instanceAlloc.vkMemroy, instanceAlloc.vkOffset));

	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(instanceBufferHandle, instanceDataDeviceAddress.deviceAddress));

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	uint32_t primitive_count = 1;
	uint32_t instance_count = (uint32_t)instances.size();

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	KVulkanGlobal::vkGetAccelerationStructureBuildSizesKHR(
		KVulkanGlobal::device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&primitive_count,
		&accelerationStructureBuildSizesInfo);

	KVulkanInitializer::CreateVkAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, accelerationStructureBuildSizesInfo, m_TopDownAS);
	KVulkanInitializer::BuildTopDownVkAccelerationStructure(accelerationStructureGeometry, accelerationStructureBuildSizesInfo, instance_count, m_TopDownAS);

	vkDestroyBuffer(KVulkanGlobal::device, instanceBufferHandle, nullptr);
	KVulkanHeapAllocator::Free(instanceAlloc);

	return true;
}

bool KVulkanAccelerationStructure::UnInit()
{
	if (m_BottomUpAS.handle)
	{
		vkDestroyBuffer(KVulkanGlobal::device, m_BottomUpAS.buffer, nullptr);
		m_BottomUpAS.buffer = VK_NULL_HANDEL;
		KVulkanGlobal::vkDestroyAccelerationStructureKHR(KVulkanGlobal::device, m_BottomUpAS.handle, nullptr);
		m_BottomUpAS.handle = VK_NULL_HANDEL;
		KVulkanHeapAllocator::Free(m_BottomUpAS.allocInfo);
	}

	if (m_TopDownAS.handle)
	{
		vkDestroyBuffer(KVulkanGlobal::device, m_TopDownAS.buffer, nullptr);
		m_TopDownAS.buffer = VK_NULL_HANDEL;
		KVulkanGlobal::vkDestroyAccelerationStructureKHR(KVulkanGlobal::device, m_TopDownAS.handle, nullptr);
		m_TopDownAS.handle = VK_NULL_HANDEL;
		KVulkanHeapAllocator::Free(m_TopDownAS.allocInfo);
	}

	m_IndexBuffer = nullptr;
	m_VertexBuffer = nullptr;

	return true;
}