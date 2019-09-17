#include "KVulkanGlobal.h"

namespace KVulkanGlobal
{
	bool deviceReady = false;
	VkDevice device = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkCommandPool graphicsCommandPool;// = nullptr;
	VkQueue graphicsQueue = nullptr;
}