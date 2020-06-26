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
}