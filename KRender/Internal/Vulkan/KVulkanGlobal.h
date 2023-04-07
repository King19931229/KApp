#pragma once

#include "KVulkanConfig.h"
#include <vector>

namespace KVulkanGlobal
{
	extern bool deviceReady;
	extern bool supportRaytrace;
	extern bool supportMeshShader;
	extern bool supportDebugMarker;

	extern VkInstance instance;
	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkCommandPool graphicsCommandPool;
	extern VkPipelineCache pipelineCache;

	extern std::vector<uint32_t> graphicsFamilyIndices;
	extern std::vector<uint32_t> computeFamilyIndices;
	extern std::vector<uint32_t> transferFamilyIndices;

	extern std::vector<VkQueue> graphicsQueues;
	extern std::vector<VkQueue> computeQueues;
	extern std::vector<VkQueue> transferQueues;

	extern VkQueue presentQueue;

	extern VkPhysicalDeviceProperties deviceProperties;
	extern VkPhysicalDeviceFeatures deviceFeatures;

	extern VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties;
	extern VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures;
	extern VkPhysicalDeviceMeshShaderPropertiesNV meshShaderFeatures;

	// Function pointers for ray tracing
	extern PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	extern PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	extern PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	extern PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	extern PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	extern PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
	extern PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	extern PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	extern PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	extern PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

	// Function pointers for mesh shader
	extern PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV;

	// Function pointers for nsight
	extern PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV;
	extern PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV;

	// Function pointers for debug marker
	extern PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag;
	extern PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName;
	extern PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin;
	extern PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd;
	extern PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert;

	// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html
	extern PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTag;
	extern PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectName;
	extern PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabel;
	extern PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabel;
	extern PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabel;
}