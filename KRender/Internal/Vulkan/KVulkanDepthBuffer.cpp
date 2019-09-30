#include "KVulkanDepthBuffer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "KVulkanInitializer.h"

KVulkanDepthBuffer::KVulkanDepthBuffer()
	: m_bStencil(false),
	m_bDeviceInit(false)
{
	ZERO_MEMORY(m_Format);	
	ZERO_MEMORY(m_DepthImage);
	ZERO_MEMORY(m_AllocInfo);
	ZERO_MEMORY(m_DepthImageView);
}

KVulkanDepthBuffer::~KVulkanDepthBuffer()
{
	ASSERT_RESULT(!m_bDeviceInit);
}

VkFormat KVulkanDepthBuffer::FindDepthFormat(bool bStencil)
{
	VkFormat format = VK_FORMAT_MAX_ENUM;
	std::vector<VkFormat> candidates;

	if(!bStencil)
	{
		candidates.push_back(VK_FORMAT_D32_SFLOAT);
	}
	candidates.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
	candidates.push_back(VK_FORMAT_D24_UNORM_S8_UINT);

	ASSERT_RESULT(KVulkanHelper::FindSupportedFormat(candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, format));

	return format;
}

bool KVulkanDepthBuffer::InitDevice(size_t uWidth, size_t uHeight, bool bStencil)
{
	ASSERT_RESULT(!m_bDeviceInit);
	if(KVulkanGlobal::deviceReady)
	{
		m_bStencil = bStencil;
		m_Format = FindDepthFormat(bStencil);

		KVulkanInitializer::CreateVkImage(static_cast<uint32_t>(uWidth),
			static_cast<uint32_t>(uHeight),
			1,
			1,
			VK_IMAGE_TYPE_2D,
			m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_DepthImage,
			m_AllocInfo);

		KVulkanInitializer::CreateVkImageView(m_DepthImage,
			m_Format,
			VK_IMAGE_ASPECT_DEPTH_BIT | (bStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0),
			1,
			m_DepthImageView);

		m_bDeviceInit = true;
		return true;
	}
	return false;
}

bool KVulkanDepthBuffer::UnInit()
{
	using namespace KVulkanGlobal;
	if(m_bDeviceInit)
	{
		vkDestroyImageView(device, m_DepthImageView, nullptr);
		KVulkanInitializer::FreeVkImage(m_DepthImage, m_AllocInfo);
		m_bDeviceInit = false;
	}
	return true;
}

bool KVulkanDepthBuffer::Resize(size_t uWidth, size_t uHeight)
{
	ASSERT_RESULT(m_bDeviceInit);
	return UnInit() && InitDevice(uWidth, uHeight, m_bStencil);
}