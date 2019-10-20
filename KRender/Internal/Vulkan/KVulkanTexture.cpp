#include "KVulkanTexture.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

KVulkanTexture::KVulkanTexture()
	: KTextureBase(),
	m_bDeviceInit(false)
{
	ZERO_MEMORY(m_AllocInfo);
	m_TextureImage = VK_NULL_HANDLE;
	m_TextureImageView = VK_NULL_HANDLE;
	m_TextureFormat = VK_FORMAT_UNDEFINED;
}

KVulkanTexture::~KVulkanTexture()
{
	ASSERT_RESULT(!m_bDeviceInit);
}

bool KVulkanTexture::InitDevice()
{
	using namespace KVulkanGlobal;
	ASSERT_RESULT(!m_bDeviceInit);

	if(m_bCreateAsRt)
	{
		VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;
		VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		m_TextureFormat = VK_FORMAT_UNDEFINED;

		ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(m_TextureType, imageType, imageViewType));
		ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(m_Format, m_TextureFormat));

		KVulkanInitializer::CreateVkImage((uint32_t)m_Width,
			(uint32_t)m_Height,
			1,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			imageType,
			m_TextureFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_AllocInfo);
		KVulkanInitializer::CreateVkImageView(m_TextureImage, m_TextureFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_TextureImageView);
		m_bDeviceInit = true;
		m_ImageData.pData = nullptr;
		return true;
	}
	else
	{
		if(m_ImageData.pData && m_ImageData.pData->GetSize() > 0)
		{
			size_t imageSize = m_ImageData.pData->GetSize();

			VkBuffer stagingBuffer;
			KVulkanHeapAllocator::AllocInfo stagingAllocInfo;

			KVulkanInitializer::CreateVkBuffer((VkDeviceSize)imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingAllocInfo);
			{
				void* pixels = m_ImageData.pData->GetData();
				assert(pixels);

				void* data = nullptr;
				VK_ASSERT_RESULT(vkMapMemory(device, stagingAllocInfo.vkMemroy, stagingAllocInfo.vkOffset, imageSize, 0, &data));
				memcpy(data, pixels, static_cast<size_t>(imageSize));
				vkUnmapMemory(device, stagingAllocInfo.vkMemroy);

				VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;
				VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
				m_TextureFormat = VK_FORMAT_UNDEFINED;

				ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(m_TextureType, imageType, imageViewType));
				ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(m_Format, m_TextureFormat));

				KVulkanInitializer::CreateVkImage((uint32_t)m_Width,
					(uint32_t)m_Height,
					(uint32_t)m_Depth,
					(uint32_t)m_Mipmaps,
					VK_SAMPLE_COUNT_1_BIT,
					imageType,
					m_TextureFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_AllocInfo);
				{
					// 先转换image layout为之后buffer拷贝数据到image作准备
					KVulkanHelper::TransitionImageLayout(m_TextureImage,
						m_TextureFormat,
						(uint32_t)m_Mipmaps,
						VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
					// 拷贝buffer数据到image
					KVulkanHelper::CopyVkBufferToVkImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));

					if(m_Mipmaps > 1)
					{
						KVulkanHelper::GenerateMipmaps(m_TextureImage, m_TextureFormat, static_cast<int32_t>(m_Width), static_cast<int32_t>(m_Height), static_cast<int32_t>(m_Mipmaps));
					}
					else
					{
						// 再转换image layout为之后shader使用image数据作准备
						KVulkanHelper::TransitionImageLayout(m_TextureImage,
							m_TextureFormat,
							(uint32_t)m_Mipmaps,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					}

					// 创建imageview
					KVulkanInitializer::CreateVkImageView(m_TextureImage, m_TextureFormat, VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)m_Mipmaps, m_TextureImageView);

					KVulkanInitializer::FreeVkBuffer(stagingBuffer, stagingAllocInfo);
				}
			}
			m_bDeviceInit = true;
			m_ImageData.pData = nullptr;
			return true;
		}
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
		KVulkanInitializer::FreeVkImage(m_TextureImage,  m_AllocInfo);
		m_bDeviceInit = false;
	}
	return true;
}

ImageView KVulkanTexture::GetImageView()
{
	ImageView ret = {m_TextureImageView, m_TextureFormat};
	return ret;
};