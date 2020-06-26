#include "KVulkanGlobal.h"

namespace KVulkanGlobal
{
	bool deviceReady = false;

	VkInstance instance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	std::mutex graphicsPoolLock;
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
	std::mutex graphicsQueueLock;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;

	uint32_t graphicsFamilyIndex = 0;
	uint32_t presentFamilyIndex = 0;
}