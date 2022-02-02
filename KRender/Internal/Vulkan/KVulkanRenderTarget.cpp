#include "KVulkanRenderTarget.h"
#include "KVulkanSwapChain.h"
#include "KVulkanTexture.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanRenderPass.h"
#include "Internal/KRenderGlobal.h"

KVulkanRenderTarget::KVulkanRenderTarget()
{
	m_FrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
}

KVulkanRenderTarget::~KVulkanRenderTarget()
{
	m_FrameBuffer = nullptr;
}

bool KVulkanRenderTarget::InitFromDepthStencil(uint32_t width, uint32_t height, uint32_t msaaCount, bool bStencil)
{
	UnInit();

	((KVulkanFrameBuffer*)m_FrameBuffer.get())->InitDepthStencil(width, height, msaaCount, bStencil);

	return true;
}

bool KVulkanRenderTarget::InitFromColor(uint32_t width, uint32_t height, uint32_t msaaCount, ElementFormat format)
{
	UnInit();

	VkFormat vkFormat = VK_FORMAT_UNDEFINED;
	ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(format, vkFormat));

	((KVulkanFrameBuffer*)m_FrameBuffer.get())->InitColor(
		vkFormat,
		TT_TEXTURE_2D,
		(uint32_t)width,
		(uint32_t)height,
		msaaCount);

	return true;
}

bool KVulkanRenderTarget::InitFromStorage(uint32_t width, uint32_t height, uint32_t mipmaps, ElementFormat format)
{
	UnInit();

	VkFormat vkFormat = VK_FORMAT_UNDEFINED;
	ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(format, vkFormat));

	((KVulkanFrameBuffer*)m_FrameBuffer.get())->InitStorage(
		vkFormat,
		(uint32_t)width,
		(uint32_t)height,
		(uint32_t)mipmaps);

	return true;
}

bool KVulkanRenderTarget::InitFromStorage3D(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, ElementFormat format)
{
	UnInit();

	VkFormat vkFormat = VK_FORMAT_UNDEFINED;
	ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(format, vkFormat));

	((KVulkanFrameBuffer*)m_FrameBuffer.get())->InitStorage3D(
		vkFormat,
		(uint32_t)width,
		(uint32_t)height,
		(uint32_t)depth,
		(uint32_t)mipmaps);

	return true;
}

bool KVulkanRenderTarget::UnInit()
{
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	((KVulkanFrameBuffer*)m_FrameBuffer.get())->UnInit();

	return true;
}

bool KVulkanRenderTarget::GetSize(size_t& width, size_t& height)
{
	width = m_FrameBuffer->GetWidth();
	height = m_FrameBuffer->GetHeight();
	return true;
}

IKFrameBufferPtr KVulkanRenderTarget::GetFrameBuffer()
{
	return m_FrameBuffer;
}

VkExtent2D KVulkanRenderTarget::GetExtend()
{
	VkExtent2D extend = {};

	size_t width = 0;
	size_t height = 0;
	if (GetSize(width, height))
	{
		extend.width = (uint32_t)width;
		extend.height = (uint32_t)height;
	}

	return extend;
}

VkSampleCountFlagBits KVulkanRenderTarget::GetMsaaFlag()
{
	return ((KVulkanFrameBuffer*)m_FrameBuffer.get())->GetMSAAFlag();
}