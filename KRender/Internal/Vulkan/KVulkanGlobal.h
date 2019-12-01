#pragma once

#include "KVulkanConfig.h"

namespace KVulkanGlobal
{
	extern bool deviceReady;

	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkCommandPool graphicsCommandPool;
	extern VkQueue graphicsQueue;
	extern VkPipelineCache pipelineCache;

	extern uint32_t graphicsFamilyIndex;
	extern uint32_t presentFamilyIndex;
}