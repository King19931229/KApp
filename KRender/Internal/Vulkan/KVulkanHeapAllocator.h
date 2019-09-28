#pragma once
#include "KVulkanConfig.h"

namespace KVulkanHeapAllocator
{
	struct AllocInfo
	{
		VkDeviceMemory vkMemroy;
		VkDeviceSize vkOffset;
		// 内部释放使用 不要修改其内容
		void* internalData;
	};

	bool Init();
	bool UnInit();
	bool Alloc(VkDeviceSize size, uint32_t memoryTypeIndex, VkMemoryPropertyFlags usage, AllocInfo& info);
	bool Free(const AllocInfo& data);
}