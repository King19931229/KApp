#include "KVulkanAccelerationStructure.h"
#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

KVulkanAccelerationStructure::KVulkanAccelerationStructure()
{
}

KVulkanAccelerationStructure::~KVulkanAccelerationStructure()
{
}

bool KVulkanAccelerationStructure::Init(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer)
{
	if (format != VF_UNKNOWN && vertexBuffer && indexBuffer)
	{
		KVulkanVertexBuffer* vulkanVertexBuffer = static_cast<KVulkanVertexBuffer*>(vertexBuffer.get());
		KVulkanIndexBuffer* vulkanIndexBuffer = static_cast<KVulkanIndexBuffer*>(indexBuffer.get());

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

		return true;
	}

	return false;
}

bool KVulkanAccelerationStructure::UnInit()
{
	return true;
}