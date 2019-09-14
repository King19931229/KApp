#pragma once

#include "vulkan/vulkan.h"

namespace KVulkanGlobal
{
	extern bool deviceReady;
	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkCommandPool graphicsCommandPool;
	extern VkQueue graphicsQueue;
}