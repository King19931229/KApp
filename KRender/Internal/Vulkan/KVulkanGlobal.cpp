#include "KVulkanGlobal.h"

namespace KVulkanGlobal
{
	bool deviceReady = false;
	bool supportRaytrace = false;
	bool supportMeshShader = false;
	bool supportDebugMarker = false;

	VkInstance instance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;
 
	VkPhysicalDeviceProperties deviceProperties = {};
	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties = {};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
	VkPhysicalDeviceMeshShaderPropertiesNV meshShaderFeatures = {};

	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = VK_NULL_HANDLE;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = VK_NULL_HANDLE;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = VK_NULL_HANDLE;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = VK_NULL_HANDLE;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = VK_NULL_HANDLE;
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR = VK_NULL_HANDLE;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = VK_NULL_HANDLE;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = VK_NULL_HANDLE;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = VK_NULL_HANDLE;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = VK_NULL_HANDLE;

	PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV = VK_NULL_HANDEL;

	PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV = VK_NULL_HANDEL;
	PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV = VK_NULL_HANDEL;

	PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag = VK_NULL_HANDLE;
	PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert = VK_NULL_HANDLE;

	PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTag = VK_NULL_HANDLE;
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectName = VK_NULL_HANDLE;
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabel = VK_NULL_HANDLE;
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabel = VK_NULL_HANDLE;
	PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabel = VK_NULL_HANDLE;

	std::vector<uint32_t> graphicsFamilyIndices = {};
	std::vector<uint32_t> computeFamilyIndices = {};
	std::vector<uint32_t> transferFamilyIndices = {};

	std::vector<VkQueue> graphicsQueues = {};
	std::vector<VkQueue> computeQueues = {};
	std::vector<VkQueue> transferQueues = {};
	VkQueue presentQueue = VK_NULL_HANDLE;
}