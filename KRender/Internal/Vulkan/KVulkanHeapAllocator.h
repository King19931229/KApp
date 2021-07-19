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

		AllocInfo()
		{
			vkMemroy = VK_NULL_HANDEL;
			vkOffset = 0;
			internalData = nullptr;
		}
	};

	bool Init();
	bool UnInit();
	bool Alloc(VkDeviceSize size, VkDeviceSize alignment, uint32_t memoryTypeIndex, VkMemoryPropertyFlags memoryUsage, VkBufferUsageFlags bufferUsage, AllocInfo& info);
	bool Free(const AllocInfo& data);
}