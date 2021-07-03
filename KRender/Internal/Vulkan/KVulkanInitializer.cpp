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
				ASSERT_RESULT(KVulkanHeapAllocator::Alloc(allocInfo.allocationSize, memRequirements.alignment, allocInfo.memoryTypeIndex, properties, heapAllocInfo));
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
		imageInfo.arrayLayers = static_cast<uint32_t>(layers);

		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = numSamples;
		imageInfo.flags = flags; 

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

			ASSERT_RESULT(KVulkanHeapAllocator::Alloc(allocInfo.allocationSize, memRequirements.alignment, allocInfo.memoryTypeIndex, properties, heapAllocInfo));
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
		VkImageViewType imageViewType,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32_t mipLevels,
		uint32_t layerCounts,
		VkImageView& vkImageView)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);

		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		// 设置image
		createInfo.image = image;
		createInfo.viewType = imageViewType;
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
		createInfo.subresourceRange.layerCount = layerCounts;
		VK_ASSERT_RESULT(vkCreateImageView(KVulkanGlobal::device, &createInfo, nullptr, &vkImageView));
	}

	void CreateVkAccelerationStructure(VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, AccelerationStructureHandle& accelerationStructure)
	{
		// Buffer and memory
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VK_ASSERT_RESULT(vkCreateBuffer(KVulkanGlobal::device, &bufferCreateInfo, nullptr, &accelerationStructure.buffer));

		VkMemoryRequirements memoryRequirements = {};
		vkGetBufferMemoryRequirements(KVulkanGlobal::device, accelerationStructure.buffer, &memoryRequirements);

		uint32_t memoryTypeIndex = 0;
		ASSERT_RESULT(KVulkanHelper::FindMemoryType(
			KVulkanGlobal::physicalDevice,
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			memoryTypeIndex));

		ASSERT_RESULT(KVulkanHeapAllocator::Alloc(memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, accelerationStructure.allocInfo));
		VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, accelerationStructure.buffer,
			accelerationStructure.allocInfo.vkMemroy,
			accelerationStructure.allocInfo.vkOffset));

		// Acceleration structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info = {};
		accelerationStructureCreate_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreate_info.buffer = accelerationStructure.buffer;
		accelerationStructureCreate_info.size = buildSizeInfo.accelerationStructureSize;
		accelerationStructureCreate_info.type = type;
		KVulkanGlobal::vkCreateAccelerationStructureKHR(KVulkanGlobal::device, &accelerationStructureCreate_info, nullptr, &accelerationStructure.handle);

		// AS device address
		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.handle;
		accelerationStructure.deviceAddress = KVulkanGlobal::vkGetAccelerationStructureDeviceAddressKHR(KVulkanGlobal::device, &accelerationDeviceAddressInfo);
	}

	void BuildBottomUpVkAccelerationStructure(VkAccelerationStructureGeometryKHR accelerationStructureGeometry, VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo, uint32_t numTriangles, AccelerationStructureHandle& accelerationStructure)
	{
		VkBuffer scratchBufferHandle = VK_NULL_HANDEL;
		VkDeviceAddress scratchBufferAddress = VK_NULL_HANDEL;

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = accelerationStructureBuildSizesInfo.buildScratchSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VK_ASSERT_RESULT(vkCreateBuffer(KVulkanGlobal::device, &bufferCreateInfo, nullptr, &scratchBufferHandle));

		VkMemoryRequirements memoryRequirements = {};
		vkGetBufferMemoryRequirements(KVulkanGlobal::device, scratchBufferHandle, &memoryRequirements);

		uint32_t memoryTypeIndex = 0;
		ASSERT_RESULT(KVulkanHelper::FindMemoryType(KVulkanGlobal::physicalDevice,
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			memoryTypeIndex));

		KVulkanHeapAllocator::AllocInfo stageAlloc;
		ASSERT_RESULT(KVulkanHeapAllocator::Alloc(memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, stageAlloc));
		VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, scratchBufferHandle, stageAlloc.vkMemroy, stageAlloc.vkOffset));

		// Buffer device address
		ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(scratchBufferHandle, scratchBufferAddress));

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = accelerationStructure.handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBufferAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = {};
		accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			KVulkanGlobal::vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);

		vkDestroyBuffer(KVulkanGlobal::device, scratchBufferHandle, nullptr);
		KVulkanHeapAllocator::Free(stageAlloc);
	}

	void BuildTopDownVkAccelerationStructure(VkAccelerationStructureGeometryKHR accelerationStructureGeometry, VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo,
		uint32_t numInstances, AccelerationStructureHandle& accelerationStructure)
	{
		VkBuffer scratchBufferHandle = VK_NULL_HANDEL;
		VkDeviceAddress scratchBufferAddress = VK_NULL_HANDEL;

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = accelerationStructureBuildSizesInfo.buildScratchSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VK_ASSERT_RESULT(vkCreateBuffer(KVulkanGlobal::device, &bufferCreateInfo, nullptr, &scratchBufferHandle));

		VkMemoryRequirements memoryRequirements = {};
		vkGetBufferMemoryRequirements(KVulkanGlobal::device, scratchBufferHandle, &memoryRequirements);

		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

		uint32_t memoryTypeIndex = 0;
		ASSERT_RESULT(KVulkanHelper::FindMemoryType(KVulkanGlobal::physicalDevice,
			memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			memoryTypeIndex));

		KVulkanHeapAllocator::AllocInfo stageAlloc;
		ASSERT_RESULT(KVulkanHeapAllocator::Alloc(memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, stageAlloc));
		VK_ASSERT_RESULT(vkBindBufferMemory(KVulkanGlobal::device, scratchBufferHandle, stageAlloc.vkMemroy, stageAlloc.vkOffset));

		// Buffer device address
		ASSERT_RESULT(KVulkanHelper::GetBufferDeviceAddress(scratchBufferHandle, scratchBufferAddress));

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = accelerationStructure.handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBufferAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = {};
		accelerationStructureBuildRangeInfo.primitiveCount = numInstances;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			KVulkanGlobal::vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);

		vkDestroyBuffer(KVulkanGlobal::device, scratchBufferHandle, nullptr);
		KVulkanHeapAllocator::Free(stageAlloc);
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

	void CopyVkBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkBufferCopy copyRegion = {};
			copyRegion.srcOffset = 0; // Optional
			copyRegion.dstOffset = 0; // Optional
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void CopyVkBufferToVkImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			// 为每行padding所用 设置为0以忽略
			region.bufferRowLength = 0;
			// 为每行padding所用 设置为0以忽略
			region.bufferImageHeight = 0;

			// VkImageSubresourceRange
			{
				// 这里不处理数组与mipmap
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
			}

			VkOffset3D imageOffset = { 0, 0, 0 };
			region.imageOffset = imageOffset;

			VkExtent3D imageExtent = { width, height, 1 };
			region.imageExtent = imageExtent;

			vkCmdCopyBufferToImage(
				commandBuffer,
				buffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
			);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void CopyVkBufferToVkImageByRegion(VkBuffer buffer, VkImage image, uint32_t layers, const BufferSubRegionCopyInfoList& copyInfo)
	{
		ASSERT_RESULT(layers > 0 && copyInfo.size() > 0);

		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			std::vector<VkBufferImageCopy> regions;
			regions.reserve(copyInfo.size());

			for (const BufferSubRegionCopyInfo& info : copyInfo)
			{
				VkBufferImageCopy region = {};

				region.bufferOffset = info.offset;
				// 为每行padding所用 设置为0以忽略
				region.bufferRowLength = 0;
				// 为每行padding所用 设置为0以忽略
				region.bufferImageHeight = 0;

				ASSERT_RESULT(info.layer < layers);

				// VkImageSubresourceRange
				{
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.mipLevel = info.mipLevel;
					// 拷贝的layer索引
					region.imageSubresource.baseArrayLayer = info.layer;
					// 拷贝多少个layer 固定为1
					region.imageSubresource.layerCount = 1;
				}

				VkOffset3D imageOffset = { 0, 0, 0 };
				region.imageOffset = imageOffset;

				VkExtent3D imageExtent = { info.width, info.height, 1 };
				region.imageExtent = imageExtent;

				regions.push_back(region);
			}

			vkCmdCopyBufferToImage(
				commandBuffer,
				buffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(regions.size()),
				regions.data()
			);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void CopyVkImageToVkImage(VkImage srcImage, VkImage dstImage, const ImageSubRegionCopyInfo& copyInfo)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkImageCopy copyRegion = {};

			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer = copyInfo.srcFaceIndex;
			copyRegion.srcSubresource.mipLevel = copyInfo.srcMipLevel;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.srcOffset = { 0, 0, 0 };

			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer = copyInfo.dstFaceIndex;
			copyRegion.dstSubresource.mipLevel = copyInfo.dstMipLevel;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.dstOffset = { 0, 0, 0 };

			copyRegion.extent.width = copyInfo.width;
			copyRegion.extent.height = copyInfo.height;
			copyRegion.extent.depth = 1;

			vkCmdCopyImage(
				commandBuffer,
				srcImage,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				dstImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void TransitionImageLayout(VkImage image, VkFormat format,
		uint32_t baseLayer, uint32_t layers,
		uint32_t baseMipLevel, uint32_t mipLevels,
		VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		TransitionImageLayoutCmdBuffer(image, format, baseLayer, layers, baseMipLevel, mipLevels, oldLayout, newLayout, commandBuffer);
		EndSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void TransitionImageLayoutCmdBuffer(VkImage image, VkFormat format,
		uint32_t baseLayer, uint32_t layers,
		uint32_t baseMipLevel, uint32_t mipLevels,
		VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		// 指定旧Layout与新Layout
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		// 这里不处理队列家族所有权转移
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		// VkImageSubresourceRange
		{
			if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

				if (KVulkanHelper::HasStencilComponent(format))
				{
					barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}
			else
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
			barrier.subresourceRange.baseMipLevel = baseMipLevel;
			barrier.subresourceRange.levelCount = mipLevels;
			barrier.subresourceRange.baseArrayLayer = baseLayer;
			barrier.subresourceRange.layerCount = layers;
		}

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldLayout)
		{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				barrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_GENERAL:
				break;

			default:
				// Other source layouts aren't handled (yet)
				ASSERT_RESULT(false && "not yet handled");
				break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newLayout)
		{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (barrier.srcAccessMask == 0)
				{
					barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_GENERAL:
				break;

			default:
				// Other source layouts aren't handled (yet)
				ASSERT_RESULT(false && "not yet handled");
				break;
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, /* srcStageMask */
			destinationStage, /* dstStageMask */
			0, /* dependencyFlags 该参数为 0 或者 VK_DEPENDENCY_BY_REGION_BIT */
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t layers, uint32_t mipLevels)
	{
		// 检查该format是否支持线性过滤
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(KVulkanGlobal::physicalDevice, format, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			ASSERT_RESULT(false && "Linear filter not supported on this format");
			return;
		}

		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			for (uint32_t layer = 0; layer < layers; ++layer)
			{
				int32_t mipWidth = texWidth;
				int32_t mipHeight = texHeight;

				for (uint32_t mipmapLevel = 1; mipmapLevel < mipLevels; mipmapLevel++)
				{
					// 先用内存屏障对上一层mipmap执行一次Transition 从Transfer目标转换到transfer源
					barrier.subresourceRange.baseMipLevel = mipmapLevel - 1;
					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

					vkCmdPipelineBarrier(commandBuffer,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr,
						0, nullptr,
						1, &barrier);

					VkImageBlit blit = {};

					VkOffset3D srcOffsets[] = { { 0, 0, 0 },{ mipWidth, mipHeight, 1 } };
					blit.srcOffsets[0] = srcOffsets[0];
					blit.srcOffsets[1] = srcOffsets[1];

					blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blit.srcSubresource.mipLevel = mipmapLevel - 1;
					blit.srcSubresource.baseArrayLayer = layer;
					blit.srcSubresource.layerCount = 1;

					VkOffset3D dstOffsets[] = { { 0, 0, 0 },{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 } };
					blit.dstOffsets[0] = dstOffsets[0];
					blit.dstOffsets[1] = dstOffsets[1];

					blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blit.dstSubresource.mipLevel = mipmapLevel;
					blit.dstSubresource.baseArrayLayer = layer;
					blit.dstSubresource.layerCount = 1;

					// 执行一次blit 把上一层 mipmap blit 到下一层 mipmap
					vkCmdBlitImage(commandBuffer,
						image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1, &blit,
						// 指定bilt时候使用线性过滤
						VK_FILTER_LINEAR);

					// 再用内存屏障对上一层mipmap执行一次Transition 从Transfer源转换到Shader可读
					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					vkCmdPipelineBarrier(commandBuffer,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						0, nullptr,
						0, nullptr,
						1, &barrier);

					if (mipWidth > 1) mipWidth /= 2;
					if (mipHeight > 1) mipHeight /= 2;
				}

				// 把遗留下来的最后一层mipmap执行Transition 从Transfer目标转换到Shader可读
				barrier.subresourceRange.baseMipLevel = mipLevels - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);
			}
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void BeginSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer)
	{
		VkCommandBufferAllocateInfo allocInfo = KVulkanInitializer::CommandBufferAllocateInfo(commandPool);

		VK_ASSERT_RESULT(vkAllocateCommandBuffers(KVulkanGlobal::device, &allocInfo, &commandBuffer));

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
	}

	void EndSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer)
	{
		VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkQueue queue = KVulkanGlobal::graphicsQueue;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FLAGS_NONE;
		fenceInfo.pNext = nullptr;

		VkFence fence;

		VK_ASSERT_RESULT(vkCreateFence(KVulkanGlobal::device, &fenceInfo, nullptr, &fence));
		// Wait for the fence to signal that command buffer has finished executing
		VK_ASSERT_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		VK_ASSERT_RESULT(vkWaitForFences(KVulkanGlobal::device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		vkDestroyFence(KVulkanGlobal::device, fence, nullptr);

		vkFreeCommandBuffers(KVulkanGlobal::device, commandPool, 1, &commandBuffer);
	}
}