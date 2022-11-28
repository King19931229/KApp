#include "KVulkanFrameBuffer.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "KVulkanCommandBuffer.h"

KVulkanFrameBuffer::KVulkanFrameBuffer()
	: m_ImageType(VK_IMAGE_TYPE_MAX_ENUM),
	m_ImageViewType(VK_IMAGE_VIEW_TYPE_MAX_ENUM),
	m_MSAAFlag(VK_SAMPLE_COUNT_1_BIT),
	m_Image(VK_NULL_HANDLE),
	m_ImageView(VK_NULL_HANDLE),
	m_MSAAImage(VK_NULL_HANDLE),
	m_MSAAImageView(VK_NULL_HANDLE),
	m_ImageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
	m_Format(VK_FORMAT_UNDEFINED),
	m_Width(0),
	m_Height(0),
	m_Depth(0),
	m_Mipmaps(0),
	m_MSAA(1),
	m_Layers(1),
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

bool KVulkanFrameBuffer::InitExternal(ExternalType type, VkImage image, VkImageView imageView, VkImageType imageType, VkImageViewType imageViewType, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t msaa)
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

	m_ImageType = imageType;
	m_ImageViewType = imageViewType;
	m_Layers = 1;

	m_ImageLayout = (type == ET_SWAPCHAIN) ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkImageCreateFlags createFlags = 0;

	if (msaa > 1)
	{
		KVulkanInitializer::CreateVkImage(m_Width,
			m_Height,
			m_Depth,
			m_Layers,
			m_Mipmaps,
			m_MSAAFlag,
			m_ImageType,
			m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			createFlags,
			m_MSAAImage, m_MSAAAllocInfo);

		KVulkanInitializer::TransitionImageLayout(m_MSAAImage, m_Format, 0, m_Layers, 0, m_Mipmaps, VK_IMAGE_LAYOUT_UNDEFINED, m_ImageLayout);
		KVulkanInitializer::CreateVkImageView(m_MSAAImage, m_ImageViewType, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, 0, m_Mipmaps, 0, 1, m_MSAAImageView);
	}

	return true;
}

bool KVulkanFrameBuffer::InitColor(VkFormat format, TextureType textureType, uint32_t width, uint32_t height, uint32_t mipmaps, uint32_t msaa)
{
	UnInit();

	m_Format = format;
	m_Width = width;
	m_Height = height;
	m_Depth = 1;
	m_Mipmaps = mipmaps;
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

	m_ImageType = VK_IMAGE_TYPE_MAX_ENUM;
	m_ImageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	m_Layers = textureType == TT_TEXTURE_CUBE_MAP ? 6 : 1;

	m_ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(textureType, m_ImageType, m_ImageViewType));

	VkImageCreateFlags createFlags = textureType == TT_TEXTURE_CUBE_MAP ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	{
		KVulkanInitializer::CreateVkImage(m_Width,
			m_Height,
			m_Depth,
			m_Layers,
			m_Mipmaps,
			VK_SAMPLE_COUNT_1_BIT,
			m_ImageType,
			m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			createFlags,
			m_Image, m_AllocInfo);

		KVulkanInitializer::TransitionImageLayout(m_Image, m_Format, 0, 1, 0, m_Mipmaps, VK_IMAGE_LAYOUT_UNDEFINED, m_ImageLayout);
		KVulkanInitializer::CreateVkImageView(m_Image, m_ImageViewType, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, 0, m_Mipmaps, 0, 1, m_ImageView);
	}

	if (msaa > 1)
	{
		KVulkanInitializer::CreateVkImage(m_Width,
			m_Height,
			m_Depth,
			m_Layers,
			m_Mipmaps,
			m_MSAAFlag,
			m_ImageType,
			m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			createFlags,
			m_MSAAImage, m_MSAAAllocInfo);

		KVulkanInitializer::TransitionImageLayout(m_MSAAImage, m_Format, 0, 1, 0, m_Mipmaps, VK_IMAGE_LAYOUT_UNDEFINED, m_ImageLayout);
		KVulkanInitializer::CreateVkImageView(m_MSAAImage, m_ImageViewType, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, 0, m_Mipmaps, 0, 1, m_MSAAImageView);
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

	m_ImageType = VK_IMAGE_TYPE_2D;
	m_ImageViewType = VK_IMAGE_VIEW_TYPE_2D;
	m_Layers = 1;

	m_ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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
		m_Depth,
		m_Layers,
		m_Mipmaps,
		m_MSAAFlag,
		m_ImageType,
		m_Format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		0,
		m_Image,
		m_AllocInfo);

	KVulkanInitializer::TransitionImageLayout(m_Image, m_Format, 0, 1, 0, m_Mipmaps, VK_IMAGE_LAYOUT_UNDEFINED, m_ImageLayout);
	KVulkanInitializer::CreateVkImageView(m_Image, m_ImageViewType, m_Format, VK_IMAGE_ASPECT_DEPTH_BIT, 0, m_Mipmaps, 0, 1, m_ImageView);

	return true;
}

bool KVulkanFrameBuffer::InitStorageInternal(VkFormat format, TextureType type, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps)
{
	UnInit();

	m_Format = format;
	m_Width = width;
	m_Height = height;
	m_Depth = depth;
	m_Mipmaps = mipmaps;
	m_MSAA = 1;
	m_External = false;

	m_MSAAFlag = VK_SAMPLE_COUNT_1_BIT;
	m_Layers = 1;

	VkImageCreateFlags createFlags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

	m_ImageType = VK_IMAGE_TYPE_MAX_ENUM;
	m_ImageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;

	m_ImageLayout = VK_IMAGE_LAYOUT_GENERAL;

	ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(type, m_ImageType, m_ImageViewType));

	{
		KVulkanInitializer::CreateVkImage(m_Width,
			m_Height,
			m_Depth,
			m_Layers,
			m_Mipmaps,
			VK_SAMPLE_COUNT_1_BIT,
			m_ImageType,
			m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			createFlags,
			m_Image, m_AllocInfo);

		KVulkanInitializer::TransitionImageLayout(m_Image, m_Format, 0, m_Layers, 0, m_Mipmaps, VK_IMAGE_LAYOUT_UNDEFINED, m_ImageLayout);
		KVulkanInitializer::CreateVkImageView(m_Image, m_ImageViewType, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, 0, m_Mipmaps, 0, 1, m_ImageView);
		// 由于Image当容器使用 需要清空数据
		KVulkanInitializer::ZeroVkImage(m_Image, m_ImageLayout, 0, m_Layers, 0, m_Mipmaps);
	}

	return true;
}

bool KVulkanFrameBuffer::InitStorage(VkFormat format, uint32_t width, uint32_t height, uint32_t mipmaps)
{
	return InitStorageInternal(format, TT_TEXTURE_2D, width, height, 1, mipmaps);
}

bool KVulkanFrameBuffer::InitStorage3D(VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps)
{
	return InitStorageInternal(format, TT_TEXTURE_3D, width, height, depth, mipmaps);
}

VkImageView KVulkanFrameBuffer::GetMipmapImageView(uint32_t startMip, uint32_t numMip)
{
	ASSERT_RESULT(startMip < 256 && numMip < 256);
	uint32_t idx = (startMip << 8) | numMip;

	VkImageView vkImageView;

	auto it = m_MipmapImageViews.find(idx);
	if (it == m_MipmapImageViews.end())
	{
		KVulkanInitializer::CreateVkImageView(m_Image, m_ImageViewType, m_Format, VK_IMAGE_ASPECT_COLOR_BIT, startMip, numMip, 0, 1, vkImageView);
		m_MipmapImageViews[idx] = vkImageView;
	}
	else
	{
		vkImageView = it->second;
	}

	return vkImageView;
}

VkImageView KVulkanFrameBuffer::GetReinterpretImageView(ElementFormat format)
{
	VkImageView imageView = VK_NULL_HANDEL;

	auto it = m_ReinterpretImageView.find(format);
	if (it != m_ReinterpretImageView.end())
	{
		imageView = it->second;
	}
	else
	{
		VkFormat vkFormat = VK_FORMAT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(format, vkFormat));

		KVulkanInitializer::CreateVkImageView(m_Image, m_ImageViewType, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT, 0, m_Mipmaps, 0, (m_ImageViewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1, imageView);
		m_ReinterpretImageView.insert({ format , imageView });
	}

	return imageView;
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

	for (auto& pair : m_MipmapImageViews)
	{
		VkImageView& view = pair.second;
		vkDestroyImageView(KVulkanGlobal::device, view, nullptr);
	}
	m_MipmapImageViews.clear();

	for (auto& pair : m_ReinterpretImageView)
	{
		vkDestroyImageView(KVulkanGlobal::device, pair.second, nullptr);
	}
	m_ReinterpretImageView.clear();

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

bool KVulkanFrameBuffer::TranslateLayout(VkCommandBuffer cmdBuffer, uint32_t baseMip, uint32_t numMip, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	// 有两种行为可能改变Layout
	// 1.Translation
	// 2.RenderPass
	KVulkanInitializer::TransitionImageLayoutCmdBuffer(m_Image, m_Format,
		0,
		(m_ImageViewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1,
		baseMip,
		numMip,
		oldLayout,
		newLayout,
		cmdBuffer);
	if (baseMip == 0 && numMip == m_Mipmaps)
	{
		m_ImageLayout = newLayout;
	}
	return true;
}

bool KVulkanFrameBuffer::Translate(IKCommandBuffer* cmd, ImageLayout oldLayout, ImageLayout newLayout)
{
	if (cmd)
	{
		KVulkanCommandBuffer* vulkanCommandBuffer = static_cast<KVulkanCommandBuffer*>(cmd);
		VkCommandBuffer vkCmdBuffer = vulkanCommandBuffer->GetVkHandle();
		VkImageLayout vkOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::ImageLayoutToVkImageLayout(oldLayout, vkOldLayout));
		VkImageLayout vkNewLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::ImageLayoutToVkImageLayout(newLayout, vkNewLayout));
		TranslateLayout(vkCmdBuffer, 0, m_Mipmaps, vkOldLayout, vkNewLayout);
		return true;
	}
	return false;
}

bool KVulkanFrameBuffer::Translate(IKCommandBuffer* cmd, ImageLayout layout)
{
	if (cmd)
	{
		KVulkanCommandBuffer* vulkanCommandBuffer = static_cast<KVulkanCommandBuffer*>(cmd);
		VkCommandBuffer vkCmdBuffer = vulkanCommandBuffer->GetVkHandle();
		VkImageLayout vkNewLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::ImageLayoutToVkImageLayout(layout, vkNewLayout));
		TranslateLayout(vkCmdBuffer, 0, m_Mipmaps, VK_IMAGE_LAYOUT_UNDEFINED, vkNewLayout);
		return true;
	}
	return false;
}

bool KVulkanFrameBuffer::TranslateMipmap(IKCommandBuffer* cmd, uint32_t mipmap, ImageLayout oldLayout, ImageLayout newLayout)
{
	if (cmd)
	{
		KVulkanCommandBuffer* vulkanCommandBuffer = static_cast<KVulkanCommandBuffer*>(cmd);
		VkCommandBuffer vkCmdBuffer = vulkanCommandBuffer->GetVkHandle();
		VkImageLayout vkOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::ImageLayoutToVkImageLayout(oldLayout, vkOldLayout));
		VkImageLayout vkNewLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::ImageLayoutToVkImageLayout(newLayout, vkNewLayout));
		TranslateLayout(vkCmdBuffer, mipmap, 1, vkOldLayout, vkNewLayout);
		return true;
	}
	return false;
}

bool KVulkanFrameBuffer::TranslateMipmap(IKCommandBuffer* cmd, uint32_t mipmap, ImageLayout layout)
{
	if (cmd)
	{
		KVulkanCommandBuffer* vulkanCommandBuffer = static_cast<KVulkanCommandBuffer*>(cmd);
		VkCommandBuffer vkCmdBuffer = vulkanCommandBuffer->GetVkHandle();
		VkImageLayout vkNewLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::ImageLayoutToVkImageLayout(layout, vkNewLayout));
		TranslateLayout(vkCmdBuffer, mipmap, 1, VK_IMAGE_LAYOUT_UNDEFINED, vkNewLayout);
		return true;
	}
	return false;
}

VkImageView KVulkanFrameBuffer::GetImageView() const
{
	return m_ImageView;
}