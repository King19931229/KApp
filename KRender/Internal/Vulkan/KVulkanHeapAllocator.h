#pragma once
#include "KVulkanConfig.h"

namespace KVulkanHeapAllocator
{
	struct AllocInfo
	{
		VkDeviceMemory vkMemroy;
		VkDeviceSize vkOffset;
		// �ڲ��ͷ�ʹ�� ��Ҫ�޸�������
		void* internalData;
	};

	bool Init();
	bool UnInit();
	bool Alloc(VkDeviceSize size, uint32_t memoryTypeIndex, VkMemoryPropertyFlags usage, AllocInfo& info);
	bool Free(const AllocInfo& data);
}