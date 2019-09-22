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
}

KVulkanTexture::~KVulkanTexture()
{

}

bool KVulkanTexture::InitDevice()
{
	using namespace KVulkanGlobal;
	m_bDeviceInit = false;
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
			VkFormat format = VK_FORMAT_UNDEFINED;

			ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(m_TextureType, imageType));
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
		vkDestroyImage(device, m_TextureImage, nullptr);
		vkFreeMemory(device, m_TextureImageMemory, nullptr);
	}
	return true;
}