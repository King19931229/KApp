#pragma once

#include "KVulkanConfig.h"
#include <mutex>

namespace KVulkanGlobal
{
	extern bool deviceReady;

	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkSurfaceKHR surface;
	extern VkCommandPool graphicsCommandPool;
	extern VkQueue graphicsQueue;
	extern VkPipelineCache pipelineCache;

	extern uint32_t graphicsFamilyIndex;
	extern uint32_t presentFamilyIndex;

	extern uint32_t currentFrameIndex;
	extern uint32_t currentFrameNum;
}