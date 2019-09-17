#pragma once
#include "Vulkan/vulkan.h"

namespace KVulkanHeapAllocator
{
	bool Init();
	bool UnInit();
	bool Alloc(VkDeviceSize size, uint32_t memoryTypeIndex, void** ppMemory);
	bool Free(void* pMemory);
}