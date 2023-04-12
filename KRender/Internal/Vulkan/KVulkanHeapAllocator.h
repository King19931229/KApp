#pragma once
#include "KVulkanConfig.h"

namespace KVulkanHeapAllocator
{
	struct AllocInfo
	{
		VkDeviceMemory vkMemroy;
		VkDeviceSize vkOffset;
		void* pMapped;
		// 内部释放使用 不要修改其内容
		void* internalData;

		AllocInfo()
		{
			vkMemroy = VK_NULL_HANDEL;
			vkOffset = 0;
			pMapped = nullptr;
			internalData = nullptr;
		}
	};

	bool Init();
	bool UnInit();
	bool Alloc(VkDeviceSize size, VkDeviceSize alignment, uint32_t memoryTypeIndex, VkMemoryPropertyFlags memoryUsage, VkBufferUsageFlags bufferUsage, bool noShared, AllocInfo& info);
	bool Free(const AllocInfo& data);
}