#pragma once

#include "KVulkanConfig.h"
#include <mutex>

namespace KVulkanGlobal
{
	extern bool deviceReady;

	extern VkDevice device;
	extern VkPhysicalDevice physicalDevice;
	extern VkSurfaceKHR surface;
	extern std::mutex graphicsPoolLock;
	extern VkCommandPool graphicsCommandPool;
	extern VkQueue graphicsQueue;
	// TODO �ɵ������ �õ�graphicsQueue�ĺ���ȫ����Ϊ���߳���ѵ
	extern std::mutex graphicsQueueLock;
	extern VkPipelineCache pipelineCache;

	extern uint32_t graphicsFamilyIndex;
	extern uint32_t presentFamilyIndex;
}