#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KConfig.h"
#include "Internal/Vulkan/KVulkanHeapAllocator.h"

#define ALLOC_TEST_COUNT 200
KVulkanHeapAllocator::AllocInfo allocResult[ALLOC_TEST_COUNT] = {0};
#define INIT_SIZE (1024 * 1024)
#include <stdlib.h>
int main()
{
	DUMP_MEMORY_LEAK_BEGIN();
	KVulkanHeapAllocator::Init();

	VkDeviceSize size = INIT_SIZE;
	VkDeviceSize totalSize = 0;
	for(int i = 0; i < ALLOC_TEST_COUNT; ++i)
	{
		size = INIT_SIZE * (rand() % 10);
		KVulkanHeapAllocator::Alloc(size, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocResult[i]);
		totalSize += size;
	}

	for(int i = 0; i < ALLOC_TEST_COUNT; ++i)
	{
		KVulkanHeapAllocator::Free(allocResult[i]);
	}

	KVulkanHeapAllocator::UnInit();
}