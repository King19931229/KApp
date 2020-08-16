#include "KVulkanFrameBuffer.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"

KVulkanFrameBuffer::KVulkanFrameBuffer()
	: m_MSAAFlag(VK_SAMPLE_COUNT_1_BIT),
	m_Image(VK_NULL_HANDLE),
	m_ImageView(VK_NULL_HANDLE),
	m_MSAAImage(VK_NULL_HANDLE),
	m_MSAAImageView(VK_NULL_HANDLE),
	m_Format(VK_FORMAT_UNDEFINED),
	m_Width(0),
	m_Height(0),
	m_Depth(0),
	m_Mipmaps(0),
	m_MSAA(1),
	m_External(true)
{
}

KVulkanFrameBuffer::~KVulkanFrameBuffer()
{
	ASSERT_RESULT(m_Image == VK_NULL_HANDLE);
	ASSERT_RESULT(m_ImageView == VK_NULL_HANDLE);
	ASSERT_RESULT(m_MSAAImage == VK_NULL_HANDLE);
	ASSERT_RESULT(m_MSAAImageView == VK_NULL_HANDLE);
}

bool KVulkanFrameBuffer::InitExternal(VkImage image, VkImageView imageView, VkFormat format,
	uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t msaa)
{
	UnInit();

	m_Image = image;
	m_ImageView = imageView;
	m_Format = format;
	m_Width = width;
	m_Height = height;
	m_Depth = depth;
	m_Mipmaps = mipmaps;
	m_MSAA = msaa;
	m_External = true;

	m_MSAAFlag = VK_SAMPLE_COUNT_1_BIT;
	if (msaa > 1)
	{
		bool supported = KVulkanHelper::QueryMSAASupport(KVulkanHelper::MST_BOTH, msaa, m_MSAAFlag);
		ASSERT_RESULT(supported);
		if (!supported)
		{
			return false;
		}
	}

	VkImageType imageType = VK_IMAGE_TYPE_2D;
	VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
	uint32_t layerCounts = 1;
	VkImageCreateFlags createFlags = 0;

	if (msaa > 1)
	{
		KVulkanInitializer::CreateVkImage(m_Width,
			m_Height,
			m_Depth,
			layerCounts,
			1,
			m_MSAAFlag,
			imageType,
			m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			createFlags,
			m_MSAAImage, m_MSAAAllocInfo);

		KVulkanInitializer::CreateVkImageView(m_MSAAImage, imageViewType, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_MSAAImageView);
		KVulkanHelper::TransitionImageLayout(m_MSAAImage, m_Format, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	return true;
}

bool KVulkanFrameBuffer::InitColor(VkFormat format, TextureType textureType, uint32_t width, uint32_t height, uint32_t msaa)
{
	UnInit();

	m_Format = format;
	m_Width = width;
	m_Height = height;
	m_Depth = 1;
	m_Mipmaps = 1;
	m_MSAA = msaa;
	m_External = false;

	m_MSAAFlag = VK_SAMPLE_COUNT_1_BIT;
	if (msaa > 1)
	{
		bool supported = KVulkanHelper::QueryMSAASupport(KVulkanHelper::MST_BOTH, msaa, m_MSAAFlag);
		ASSERT_RESULT(supported);
		if (!supported)
		{
			return false;
		}
	}

	VkImageType imageType = VK_IMAGE_TYPE_2D;
	VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
	uint32_t layerCounts = textureType == TT_TEXTURE_CUBE_MAP ? 6 : 1;
	VkImageCreateFlags createFlags = textureType == TT_TEXTURE_CUBE_MAP ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(textureType, imageType, imageViewType));

	{
		KVulkanInitializer::CreateVkImage(m_Width,
			m_Height,
			m_Depth,
			layerCounts,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			imageType,
			m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			createFlags,
			m_Image, m_AllocInfo);

		KVulkanInitializer::CreateVkImageView(m_Image, imageViewType, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_ImageView);
	}

	if (msaa > 1)
	{
		KVulkanInitializer::CreateVkImage(m_Width,
			m_Height,
			m_Depth,
			layerCounts,
			1,
			m_MSAAFlag,
			imageType,
			m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			createFlags,
			m_MSAAImage, m_MSAAAllocInfo);

		KVulkanInitializer::CreateVkImageView(m_MSAAImage, imageViewType, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_MSAAImageView);
		KVulkanHelper::TransitionImageLayout(m_MSAAImage, m_Format, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	return true;
}

bool KVulkanFrameBuffer::InitDepthStencil(uint32_t width, uint32_t height, uint32_t msaa, bool stencil)
{
	UnInit();

	KVulkanHelper::FindBestDepthFormat(stencil, m_Format);
	m_Width = width;
	m_Height = height;
	m_Depth = 1;
	m_Mipmaps = 1;
	m_MSAA = msaa;
	m_External = false;

	m_MSAAFlag = VK_SAMPLE_COUNT_1_BIT;
	if (msaa > 1)
	{
		bool supported = KVulkanHelper::QueryMSAASupport(KVulkanHelper::MST_BOTH, msaa, m_MSAAFlag);
		ASSERT_RESULT(supported);
		if (!supported)
		{
			return false;
		}
	}

	KVulkanInitializer::CreateVkImage(m_Width,
		m_Height,
		1,
		1, 1,
		m_MSAAFlag,
		VK_IMAGE_TYPE_2D,
		m_Format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		0,
		m_Image,
		m_AllocInfo);

	KVulkanInitializer::CreateVkImageView(m_Image,
		VK_IMAGE_VIEW_TYPE_2D,
		m_Format,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		1,
		m_ImageView);

	return true;
}

bool KVulkanFrameBuffer::UnInit()
{
	if (!m_External)
	{
		if (m_ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(KVulkanGlobal::device, m_ImageView, nullptr);
		}
		if (m_Image != VK_NULL_HANDLE)
		{
			KVulkanInitializer::FreeVkImage(m_Image, m_AllocInfo);
		}
	}

	m_ImageView = VK_NULL_HANDLE;
	m_Image = VK_NULL_HANDLE;

	if (m_MSAAImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(KVulkanGlobal::device, m_MSAAImageView, nullptr);
		m_MSAAImageView = VK_NULL_HANDLE;
	}
	if (m_MSAAImage != VK_NULL_HANDLE)
	{
		KVulkanInitializer::FreeVkImage(m_MSAAImage, m_MSAAAllocInfo);
		m_MSAAImage = VK_NULL_HANDLE;
	}

	return true;
}