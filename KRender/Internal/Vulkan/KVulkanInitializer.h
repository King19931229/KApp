#pragma once
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"
#include <vector>

namespace KVulkanInitializer
{
	struct BufferSubRegionCopyInfo
	{
		uint32_t offset;
		uint32_t width;
		uint32_t height;
		uint32_t mipLevel;
		uint32_t layer;
	};
	typedef std::vector<BufferSubRegionCopyInfo> BufferSubRegionCopyInfoList;

	struct ImageSubRegionCopyInfo
	{
		uint32_t width;
		uint32_t height;
		uint32_t srcFaceIndex;
		uint32_t dstFaceIndex;
		uint32_t srcMipLevel;
		uint32_t dstMipLevel;
	};

	struct ImageBlitInfo
	{
		uint32_t size[3];
		uint32_t layerCount;
	};

	struct AccelerationStructureHandle
	{
		KVulkanHeapAllocator::AllocInfo allocInfo;
		VkAccelerationStructureKHR handle;
		VkDeviceAddress deviceAddress;
		VkBuffer buffer;
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo;

		AccelerationStructureHandle()
		{
			handle = VK_NULL_HANDEL;
			deviceAddress = VK_NULL_HANDEL;
			buffer = VK_NULL_HANDEL;
			sizeInfo = {};
		}
	};

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
		uint32_t baseLevel,
		uint32_t mipLevels,
		uint32_t baseLayer,
		uint32_t layerCount,
		VkImageView& vkImageView);

	void CreateVkAccelerationStructure(VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, AccelerationStructureHandle& accelerationStructure);
	void BuildBottomUpVkAccelerationStructure(VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo, VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo, uint32_t numTriangles);
	void BuildTopDownVkAccelerationStructure(VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo, VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo, uint32_t numInstances);

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool);

	void CopyVkBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CopyVkBufferToVkImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CopyVkBufferToVkImageByRegion(VkBuffer buffer, VkImage image, uint32_t layers, const BufferSubRegionCopyInfoList& copyInfo);

	void CopyVkImageToVkImage(VkImage srcImage, VkImage dstImage, const ImageSubRegionCopyInfo& copyInfo);

	void BlitVkImageToVkImage(VkImage srcImage, VkImage dstImage, const ImageBlitInfo& blitInfo);

	void ZeroVkImage(VkImage image, VkImageLayout imageLayout, uint32_t baseLayer, uint32_t layers, uint32_t baseMipLevel, uint32_t mipLevels);

	void TransitionImageLayout(VkImage image, VkFormat format,
		uint32_t baseLayer, uint32_t layers,
		uint32_t baseMipLevel, uint32_t mipLevels,
		VkImageLayout oldLayout, VkImageLayout newLayout);

	void TransitionImageLayoutCmdBuffer(VkImage image, VkFormat format,
		uint32_t baseLayer, uint32_t layers,
		uint32_t baseMipLevel, uint32_t mipLevels,
		VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer);

	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, int32_t texDepth, uint32_t layers, uint32_t mipLevels);

	void BeginSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
	void EndSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);

	void CreateStorageBuffer(VkDeviceSize size, const void* pSrcData, VkBuffer& vkBuffer, KVulkanHeapAllocator::AllocInfo& heapAllocInfo);
	void DestroyStorageBuffer(VkBuffer& vkBuffer, KVulkanHeapAllocator::AllocInfo& heapAllocInfo);

	VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount = 1);

	VkDescriptorBufferInfo CreateDescriptorBufferIntfo(VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
	VkDescriptorImageInfo CreateDescriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);

	VkWriteDescriptorSet CreateDescriptorAccelerationStructureWrite(const VkWriteDescriptorSetAccelerationStructureKHR* descriptorAccelerationStructureInfo, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t descriptorCount = 1);
	VkWriteDescriptorSet CreateDescriptorImageWrite(const VkDescriptorImageInfo* imageInfo, VkDescriptorType descriptorType, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t descriptorCount = 1);
	VkWriteDescriptorSet CreateDescriptorBufferWrite(const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType descriptorType, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t descriptorCount = 1);
}