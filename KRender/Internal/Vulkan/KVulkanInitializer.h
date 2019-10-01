#pragma once
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

namespace KVulkanInitializer
{
	void CreateVkBuffer(VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& vkBuffer,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo);

	void FreeVkBuffer(VkBuffer& vkBuffer,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo);

	void CreateVkImage(uint32_t width,
		uint32_t height,
		uint32_t depth,
		uint32_t mipLevels,
		VkSampleCountFlagBits numSamples,
		VkImageType imageType,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo);

	void FreeVkImage(VkImage& image,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo);

	void CreateVkImageView(VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32_t mipLevels,
		VkImageView& vkImageView);

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool);
}