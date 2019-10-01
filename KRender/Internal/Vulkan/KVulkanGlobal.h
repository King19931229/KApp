#pragma once

#include "KVulkanConfig.h"

namespace KVulkanGlobal
{
	struct MSAASupport
	{
		bool sample_1;

	};


	extern bool deviceReady;
	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkCommandPool graphicsCommandPool;
	extern VkQueue graphicsQueue;
}