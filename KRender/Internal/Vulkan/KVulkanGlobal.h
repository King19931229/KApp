#pragma once

#include "KVulkanConfig.h"
#include <mutex>

namespace KVulkanGlobal
{
	extern bool deviceReady;

	extern VkInstance instance;
	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkCommandPool graphicsCommandPool;
	extern VkQueue graphicsQueue;
	extern VkPipelineCache pipelineCache;

	extern uint32_t graphicsFamilyIndex;

	// Function pointers for ray tracing related stuff
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
}