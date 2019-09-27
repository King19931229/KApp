#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

namespace KVulkanInitializer
{
	void CreateVkBuffer(VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& vkBuffer,
		VkDeviceMemory& vkBufferMemory)
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
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				allocInfo.memoryTypeIndex));
			{
				// TODO 使用内存池
				VK_ASSERT_RESULT(vkAllocateMemory(KVulkanGlobal::device, &allocInfo, nullptr, &vkBufferMemory));
				{
					VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, vkBuffer, vkBufferMemory, 0));	
				}
			}
		}
	}

	void CreateVkImage(uint32_t width,
		uint32_t height,
		uint32_t depth,
		VkImageType imageType,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		imageInfo.imageType = imageType;
		imageInfo.extent.width = static_cast<uint32_t>(width);
		imageInfo.extent.height = static_cast<uint32_t>(height);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;

		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				allocInfo.memoryTypeIndex));

			VK_ASSERT_RESULT(vkAllocateMemory(KVulkanGlobal::device, &allocInfo, nullptr, &imageMemory));
			VK_ASSERT_RESULT(vkBindImageMemory(KVulkanGlobal::device, image, imageMemory, 0));
		}
	}

	void CreateVkImageView(VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
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
		createInfo.subresourceRange.levelCount = 1;
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