#pragma once
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"
#include <vector>

namespace KVulkanInitializer
{
	struct SubRegionCopyInfo
	{
		uint32_t offset;
		uint32_t width;
		uint32_t height;
		uint32_t mipLevel;
		uint32_t layer;
	};
	typedef std::vector<SubRegionCopyInfo> SubRegionCopyInfoList;

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
		uint32_t layers,
		uint32_t mipLevels,
		VkSampleCountFlagBits numSamples,
		VkImageType imageType,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImageCreateFlags flags,
		VkImage& image,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo);

	void FreeVkImage(VkImage& image,
		KVulkanHeapAllocator::AllocInfo& heapAllocInfo);

	void CreateVkImageView(VkImage image,
		VkImageViewType imageViewType,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32_t mipLevels,
		VkImageView& vkImageView);

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool);

	void CopyVkBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CopyVkBufferToVkImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CopyVkBufferToVkImageByRegion(VkBuffer buffer, VkImage image, uint32_t layers, const SubRegionCopyInfoList& copyInfo);

	void TransitionImageLayout(VkImage image, VkFormat format, uint32_t layers, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout);
	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t layers, uint32_t mipLevels);

	void BeginSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
	void EndSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
}