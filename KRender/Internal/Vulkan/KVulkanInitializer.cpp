#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

namespace KVulkanInitializer
{
	void CreateVkBuffer(VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& vkBuffer,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_ASSERT_RESULT(vkCreateBuffer(KVulkanGlobal::device, &bufferInfo, nullptr, &vkBuffer));
		{
			VkMemoryRequirements memRequirements = {};
			vkGetBufferMemoryRequirements(KVulkanGlobal::device, vkBuffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = UINT32_MAX;

			ASSERT_RESULT(KVulkanHelper::FindMemoryType(
				KVulkanGlobal::physicalDevice,
				memRequirements.memoryTypeBits,
				properties,
				allocInfo.memoryTypeIndex));
			{
				ASSERT_RESULT(KVulkanHeapAllocator::Alloc(allocInfo.allocationSize, allocInfo.memoryTypeIndex, properties, heapAllocInfo));
				VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, vkBuffer, heapAllocInfo.vkMemroy, heapAllocInfo.vkOffset));
			}
		}
	}

	void FreeVkBuffer(VkBuffer& vkBuffer, KVulkanHeapAllocator::AllocInfo& heapAllocInfo)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		vkDestroyBuffer(KVulkanGlobal::device, vkBuffer, nullptr);
		KVulkanHeapAllocator::Free(heapAllocInfo);
	}

	void CreateVkImage(uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t mipLevels,
		VkImageType imageType,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		imageInfo.imageType = imageType;
		imageInfo.extent.width = static_cast<uint32_t>(width);
		imageInfo.extent.height = static_cast<uint32_t>(height);
		imageInfo.extent.depth = static_cast<uint32_t>(depth);
		imageInfo.mipLevels = static_cast<uint32_t>(mipLevels);
		imageInfo.arrayLayers = 1;

		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0; 

		VK_ASSERT_RESULT(vkCreateImage(KVulkanGlobal::device, &imageInfo, nullptr, &image));
		{
			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(KVulkanGlobal::device, image, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;

			ASSERT_RESULT(KVulkanHelper::FindMemoryType(
				KVulkanGlobal::physicalDevice,
				memRequirements.memoryTypeBits,
				properties,
				allocInfo.memoryTypeIndex));

			ASSERT_RESULT(KVulkanHeapAllocator::Alloc(allocInfo.allocationSize, allocInfo.memoryTypeIndex, properties, heapAllocInfo));
			VK_ASSERT_RESULT(vkBindImageMemory(KVulkanGlobal::device, image, heapAllocInfo.vkMemroy, heapAllocInfo.vkOffset));
		}
	}

	void FreeVkImage(VkImage& image,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		vkDestroyImage(KVulkanGlobal::device, image, nullptr);
		KVulkanHeapAllocator::Free(heapAllocInfo);
	}

	void CreateVkImageView(VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32_t mipLevels,
		VkImageView& vkImageView)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);

		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		// 设置image
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		// format与交换链format同步
		createInfo.format = format;
		// 保持默认rgba映射行为
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		// 指定View访问范围
		createInfo.subresourceRange.aspectMask = aspectFlags;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = mipLevels;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		VK_ASSERT_RESULT(vkCreateImageView(KVulkanGlobal::device, &createInfo, nullptr, &vkImageView));
	}

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = pool;
		allocInfo.commandBufferCount = 1;
		return allocInfo;
	}
}