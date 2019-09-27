#pragma once
#include "KVulkanConfig.h"

namespace KVulkanInitializer
{
	void CreateVkBuffer(VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& vkBuffer,
		VkDeviceMemory& vkBufferMemory);

	void CreateVkImage(uint32_t width,
		uint32_t height,
		uint32_t depth,
		VkImageType imageType,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory);

	void CreateVkImageView(VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkImageView& vkImageView);

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool);
}