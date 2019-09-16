#include "KVulkanGlobal.h"

namespace KVulkanGlobal
{
	bool deviceReady = false;
	VkDevice device = (VkDevice)nullptr;
	VkPhysicalDevice physicalDevice = (VkPhysicalDevice)nullptr;
	VkCommandPool graphicsCommandPool = (VkCommandPool)nullptr;
	VkQueue graphicsQueue = (VkQueue)nullptr;
}