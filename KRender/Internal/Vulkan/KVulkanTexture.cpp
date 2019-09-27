#include "KVulkanTextrue.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

KVulkanTexture::KVulkanTexture()
	: KTextureBase(),
	m_bDeviceInit(false)
{
	ZERO_MEMORY(m_TextureImage);
	ZERO_MEMORY(m_TextureImageMemory);
	ZERO_MEMORY(m_TextureImageView);
}

KVulkanTexture::~KVulkanTexture()
{

}

bool KVulkanTexture::InitDevice()
{
	using namespace KVulkanGlobal;
	ASSERT_RESULT(!m_bDeviceInit);
	if(m_ImageData.pData && m_ImageData.pData->GetSize() > 0)
	{
		size_t imageSize = m_ImageData.pData->GetSize();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;	

		KVulkanInitializer::CreateVkBuffer((VkDeviceSize)imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);
		{
			void* pixels = m_ImageData.pData->GetData();
			assert(pixels);

			void* data = nullptr;
			VK_ASSERT_RESULT(vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data));
			memcpy(data, pixels, static_cast<size_t>(imageSize));
			vkUnmapMemory(device, stagingBufferMemory);

			VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;
			VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			VkFormat format = VK_FORMAT_UNDEFINED;

			ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(m_TextureType, imageType, imageViewType));
			ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(m_Format, format));

			KVulkanInitializer::CreateVkImage((uint32_t)m_Width,
				(uint32_t)m_Height,
				(uint32_t)m_Depth,
				imageType,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);
			{
				// 先转换image layout为之后buffer拷贝数据到image作准备
				KVulkanHelper::TransitionImageLayout(m_TextureImage,
					format,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				// 拷贝buffer数据到image
				KVulkanHelper::CopyVkBufferToVkImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));
				// 再转换image layout为之后shader使用image数据作准备
				KVulkanHelper::TransitionImageLayout(m_TextureImage,
					format,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				// 创建imageview
				{
					VkImageViewCreateInfo viewInfo = {};
					viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					viewInfo.image = m_TextureImage;
					viewInfo.viewType = imageViewType;
					viewInfo.format = format;
					viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					viewInfo.subresourceRange.baseMipLevel = 0;
					viewInfo.subresourceRange.levelCount = 1;
					viewInfo.subresourceRange.baseArrayLayer = 0;
					viewInfo.subresourceRange.layerCount = 1;

					VK_ASSERT_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &m_TextureImageView));
				}

				vkDestroyBuffer(device, stagingBuffer, nullptr);
				vkFreeMemory(device, stagingBufferMemory, nullptr);
			}
		}
		m_bDeviceInit = true;
		m_ImageData.pData = nullptr;
		return true;
	}
	return false;
}

bool KVulkanTexture::UnInit()
{
	using namespace KVulkanGlobal;
	KTextureBase::UnInit();
	if(m_bDeviceInit)
	{
		vkDestroyImageView(device, m_TextureImageView, nullptr);
		vkDestroyImage(device, m_TextureImage, nullptr);
		vkFreeMemory(device, m_TextureImageMemory, nullptr);
	}
	return true;
}