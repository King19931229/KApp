#include "KVulkanGlobal.h"

namespace KVulkanGlobal
{
	bool deviceReady = false;

	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;

	uint32_t graphicsFamilyIndex = 0;
	uint32_t presentFamilyIndex = 0;
}