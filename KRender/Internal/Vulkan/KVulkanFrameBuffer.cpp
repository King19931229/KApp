#include "KVulkanFrameBuffer.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanQueue.h"

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
		// 清空数据
		KVulkanInitializer::ZeroVkImage(m_Image, m_ImageLayout, 0, m_Layers, 0, m_Mipmaps);
	}

	return true;
}

bool KVulkanFrameBuffer::InitReadback(VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps)
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

	m_ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	ASSERT_RESULT(KVulkanHelper::TextureTypeToVkImageType(depth > 1 ? TT_TEXTURE_3D : TT_TEXTURE_2D, m_ImageType, m_ImageViewType));

	{
		KVulkanInitializer::CreateVkImage(m_Width,
			m_Height,
			m_Depth,
			m_Layers,
			m_Mipmaps,
			VK_SAMPLE_COUNT_1_BIT,
			m_ImageType,
			m_Format,
			VK_IMAGE_TILING_LINEAR,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			createFlags,
			m_Image, m_AllocInfo);

		KVulkanInitializer::TransitionImageLayout(m_Image, m_Format, 0, m_Layers, 0, m_Mipmaps, VK_IMAGE_LAYOUT_UNDEFINED, m_ImageLayout);
		// 没有ImageView
		m_ImageView = VK_NULL_HANDEL;
		// 清空数据
		KVulkanInitializer::ZeroVkImage(m_Image, m_ImageLayout, 0, m_Layers, 0, m_Mipmaps);
	}

	return true;
}

bool KVulkanFrameBuffer::CopyToReadback(IKFrameBuffer* framebuffer)
{
	if (framebuffer && framebuffer->IsReadback())
	{
		KVulkanFrameBuffer* src = this;
		KVulkanFrameBuffer* dest = static_cast<KVulkanFrameBuffer*>(framebuffer);
		
		ASSERT_RESULT(src->GetWidth() == dest->GetWidth());
		ASSERT_RESULT(src->GetHeight() == dest->GetHeight());
		ASSERT_RESULT(src->GetDepth() == dest->GetDepth());
		ASSERT_RESULT(src->GetMipmaps() == dest->GetMipmaps());
		ASSERT_RESULT(src->GetForamt() == dest->GetForamt());

		bool supportsBlit = true;
		// Check blit support for source and destination
		VkFormatProperties formatProps;

		// Check if the device supports blitting from optimal images
		vkGetPhysicalDeviceFormatProperties(KVulkanGlobal::physicalDevice ,src->m_Format, &formatProps);
		if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
		{
			supportsBlit = false;
		}

		// Check if the device supports blitting to linear images 
		vkGetPhysicalDeviceFormatProperties(KVulkanGlobal::physicalDevice, dest->m_Format, &formatProps);
		if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
		{
			supportsBlit = false;
		}

		KVulkanInitializer::TransitionImageLayout(src->m_Image, src->m_Format, 0, src->m_Layers, 0, src->m_Mipmaps, src->m_ImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		KVulkanInitializer::TransitionImageLayout(dest->m_Image, dest->m_Format, 0, dest->m_Layers, 0, dest->m_Mipmaps, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		if (supportsBlit)
		{
			KVulkanInitializer::ImageBlitInfo blitInfo = {};
			blitInfo.size[0] = src->GetWidth();
			blitInfo.size[1] = dest->GetWidth();
			blitInfo.size[2] = dest->GetDepth();

			KVulkanInitializer::BlitVkImageToVkImage(src->m_Image, dest->m_Image, blitInfo);
		}
		else
		{
			KVulkanInitializer::ImageSubRegionCopyInfo copyInfo = {};

			copyInfo.width = src->GetWidth();
			copyInfo.height = src->GetHeight();
			copyInfo.srcMipLevel = 0;
			copyInfo.srcFaceIndex = 0;
			copyInfo.dstMipLevel = 0;
			copyInfo.dstFaceIndex = 0;

			KVulkanInitializer::CopyVkImageToVkImage(src->m_Image, dest->m_Image, copyInfo);
		}

		KVulkanInitializer::TransitionImageLayout(src->m_Image, src->m_Format, 0, src->m_Layers, 0, src->m_Mipmaps, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src->m_ImageLayout);
		KVulkanInitializer::TransitionImageLayout(dest->m_Image, dest->m_Format, 0, dest->m_Layers, 0, dest->m_Mipmaps, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

		return true;
	}
	return false;
}

bool KVulkanFrameBuffer::Readback(void* pDest, size_t size)
{
	if (pDest && IsReadback())
	{
		if (m_AllocInfo.pMapped)
		{
			memcpy(pDest, m_AllocInfo.pMapped, size);
		}
		else
		{
			void* pSrc = nullptr;
			if (vkMapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, size, 0, (void**)&pSrc) == VK_SUCCESS)
			{
				memcpy(pDest, pSrc, size);
				vkUnmapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy);
			}
		}
		return true;
	}
	return false;
}

bool KVulkanFrameBuffer::InitStorage2D(VkFormat format, uint32_t width, uint32_t height, uint32_t mipmaps)
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

bool KVulkanFrameBuffer::SetDebugName(const char* name)
{
	if (name && m_Image != VK_NULL_HANDEL)
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_Image, VK_OBJECT_TYPE_IMAGE, name);
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_ImageView, VK_OBJECT_TYPE_IMAGE_VIEW, (std::string(name) + "(imageview)").c_str());

		if (m_MSAAImage != VK_NULL_HANDEL)
		{
			KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_MSAAImage, VK_OBJECT_TYPE_IMAGE, (std::string(name) + "_msaa").c_str());
			KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_MSAAImageView, VK_OBJECT_TYPE_IMAGE_VIEW, (std::string(name) + "_msaa(imageview)").c_str());
		}

		return true;
	}
	return false;
}

bool KVulkanFrameBuffer::TranslateLayoutImpl(VkCommandBuffer cmdBuffer, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, uint32_t baseMip, uint32_t numMip, VkPipelineStageFlags srcStages, VkPipelineStageFlags dstStages, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	KVulkanInitializer::TransitionImageLayoutCmdBuffer(m_Image, m_Format,
		0,
		(m_ImageViewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1,
		baseMip, numMip,
		srcQueueFamilyIndex, dstQueueFamilyIndex,
		srcStages, dstStages,
		oldLayout, newLayout,
		cmdBuffer);

	if (baseMip == 0 && numMip == m_Mipmaps)
	{
		m_ImageLayout = newLayout;
	}

	return true;
}

bool KVulkanFrameBuffer::Translate(IKCommandBuffer* cmd, IKQueue* srcQueue, IKQueue* dstQueue, uint32_t baseMip, uint32_t numMip, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	if (cmd)
	{
		KVulkanCommandBuffer* vulkanCommandBuffer = static_cast<KVulkanCommandBuffer*>(cmd);
		VkCommandBuffer vkCmdBuffer = vulkanCommandBuffer->GetVkHandle();

		VkPipelineStageFlags vkSrcStageFlags = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
		VkImageLayout vkOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::PipelineStagesToVkPipelineStageFlags(srcStages, vkSrcStageFlags));
		ASSERT_RESULT(KVulkanHelper::ImageLayoutToVkImageLayout(oldLayout, vkOldLayout));

		VkPipelineStageFlags vkDstStageFlags = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
		VkImageLayout vkNewLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ASSERT_RESULT(KVulkanHelper::PipelineStagesToVkPipelineStageFlags(dstStages, vkDstStageFlags));
		ASSERT_RESULT(KVulkanHelper::ImageLayoutToVkImageLayout(newLayout, vkNewLayout));

		uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		if (srcQueue)
		{
			srcQueueFamilyIndex = ((KVulkanQueue*)srcQueue)->GetQueueFamilyIndex();
		}
		if (dstQueue)
		{
			dstQueueFamilyIndex = ((KVulkanQueue*)dstQueue)->GetQueueFamilyIndex();
		}

		TranslateLayoutImpl(vkCmdBuffer, srcQueueFamilyIndex, dstQueueFamilyIndex, baseMip, numMip, vkSrcStageFlags, vkDstStageFlags, vkOldLayout, vkNewLayout);
		return true;
	}
	return false;
}

VkImageView KVulkanFrameBuffer::GetImageView() const
{
	return m_ImageView;
}