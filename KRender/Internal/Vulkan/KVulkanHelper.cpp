#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"

namespace KVulkanHelper
{
	bool FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& idx)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			// 1.typeFilter是否与内存类型兼容
			// 2.propertyFlags内存属性是否完整包含所需properties
			if ((typeFilter & (1 << i))	&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				idx = i;
				return true;
			}
		}
		return false;
	}

	bool TextureTypeToVkImageType(TextureType textureType, VkImageType& imageType)
	{
		switch (textureType)
		{
		case TT_TEXTURE_2D:
			imageType = VK_IMAGE_TYPE_2D;
			return true;
		case TT_COUNT:
		default:
			imageType = VK_IMAGE_TYPE_MAX_ENUM;
			assert(false && "Unknown texture type");
			return false;
		}
	}

	bool ElementFormatToVkFormat(ElementFormat elementFormat, VkFormat& vkFormat)
	{
		switch (elementFormat)
		{
		case EF_R8GB8BA8_UNORM:
			vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
			return true;
		case EF_R8G8B8A8_SNORM:
			vkFormat = VK_FORMAT_R8G8B8A8_SNORM;
			return true;
		case EF_R8GB8B8_UNORM:
			vkFormat = VK_FORMAT_R8G8B8_UNORM;
			return true;
		case EF_R16_FLOAT:
			vkFormat = VK_FORMAT_R16_SFLOAT;
			return true;
		case EF_R16G16_FLOAT:
			vkFormat = VK_FORMAT_R16G16_SFLOAT;
			return true;
		case EF_R16G16B16_FLOAT:
			vkFormat = VK_FORMAT_R16G16B16_SFLOAT;
			return true;
		case EF_R16G16B16A16_FLOAT:
			vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
			return true;
		case EF_R32_FLOAT:
			vkFormat = VK_FORMAT_R32_SFLOAT;
			return true;
		case EF_R32G32_FLOAT:
			vkFormat = VK_FORMAT_R32G32_SFLOAT;
			return true;
		case EF_R32G32B32_FLOAT:
			vkFormat = VK_FORMAT_R32G32B32_SFLOAT;
			return true;
		case EF_R32G32B32A32_FLOAT:
			vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
			return true;
		case EF_R32_UINT:
			vkFormat = VK_FORMAT_R32_UINT;
			return true;
		default:
			vkFormat = VK_FORMAT_UNDEFINED;
			assert(false && "Unknown format");
			return false;
		}
	}

	bool PopulateInputBindingDescription(const KVertexDefinition::VertexBindingDetail* pData, uint32_t nCount, VulkanBindingDetailList& detailList)
	{
		using KVertexDefinition::VertexDetail;
		using KVertexDefinition::VertexBindingDetail;
		using KVertexDefinition::VertexSemanticDetail;
		using KVertexDefinition::VertexSemanticDetailList;

		if(pData == nullptr || nCount == 0)
			return false;

		detailList.clear();
		for(uint32_t idx = 0; idx < nCount; ++idx)
		{
			const VertexBindingDetail& detail = pData[idx];

			for(VertexFormat format : detail.formats)
			{
				VulkanBindingDetail bindingDetail;

				VkVertexInputBindingDescription& bindingDescription = bindingDetail.bindingDescription;				
				std::vector<VkVertexInputAttributeDescription>& attributeDescriptions = bindingDetail.attributeDescriptions;

				const VertexDetail& vertexDetail = KVertexDefinition::GetVertexDetail(format);

				// 构造VkVertexInputBindingDescription
				bindingDescription.binding = idx;
				bindingDescription.stride = (uint32_t)vertexDetail.vertexSize;
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				// 构造VkVertexInputAttributeDescription
				const VertexSemanticDetailList& semanticDetailList = vertexDetail.semanticDetails;
				for(const VertexSemanticDetail& semanticDetail : semanticDetailList)
				{
					VkVertexInputAttributeDescription attributeDescription = {};
					VkFormat vkFormat = VK_FORMAT_UNDEFINED;
					ElementFormatToVkFormat(semanticDetail.elementFormat, vkFormat);

					attributeDescription.binding = idx;
					// 语意枚举值即为绑定位置
					attributeDescription.location = semanticDetail.semantic;
					attributeDescription.format = vkFormat;
					attributeDescription.offset = (uint32_t)semanticDetail.offset;

					attributeDescriptions.push_back(attributeDescription);
				}

				detailList.push_back(bindingDetail);
			}
		}

		return true;
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
		EndSingleTimeCommand(KVulkanGlobal::graphicsQueue, KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void CopyVkBufferToVkImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			VkOffset3D imageOffset = {0, 0, 0};
			region.imageOffset = imageOffset;

			VkExtent3D imageExtent = {width, height, 1};
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
		EndSingleTimeCommand(KVulkanGlobal::graphicsQueue, KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;

			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			barrier.srcAccessMask = 0; // TODO
			barrier.dstAccessMask = 0; // TODO

			vkCmdPipelineBarrier(
				commandBuffer,
				0 /* TODO */, 0 /* TODO */,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
				);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsQueue, KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void BeginSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer)
	{
		VkCommandBufferAllocateInfo allocInfo = KVulkanInitializer::CommandBufferAllocateInfo(commandPool);

		VK_ASSERT_RESULT(vkAllocateCommandBuffers(KVulkanGlobal::device, &allocInfo, &commandBuffer));

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));;
	}

	void EndSingleTimeCommand(VkQueue queue, VkCommandPool commandPool, VkCommandBuffer& commandBuffer)
	{
		using namespace KVulkanGlobal;

		VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VK_ASSERT_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VK_ASSERT_RESULT(vkQueueWaitIdle(queue));

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}
}