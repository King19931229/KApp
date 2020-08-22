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
	: m_DepthStencil(false)
{
}

KVulkanRenderTarget::~KVulkanRenderTarget()
{
	ASSERT_RESULT(m_FrameBuffer == nullptr);
}

bool KVulkanRenderTarget::InitFromDepthStencil(uint32_t width, uint32_t height, bool bStencil)
{
	UnInit();

	m_FrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
	((KVulkanFrameBuffer*)m_FrameBuffer.get())->InitDepthStencil((uint32_t)width, (uint32_t)height, 1, bStencil);

	m_DepthStencil = true;

	return true;
}

bool KVulkanRenderTarget::InitFromColor(uint32_t width, uint32_t height, unsigned short uMsaaCount, ElementFormat format)
{
	UnInit();

	m_FrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());

	VkFormat vkFormat = VK_FORMAT_UNDEFINED;
	ASSERT_RESULT(KVulkanHelper::ElementFormatToVkFormat(format, vkFormat));

	((KVulkanFrameBuffer*)m_FrameBuffer.get())->InitColor(
		vkFormat,
		TT_TEXTURE_2D,
		(uint32_t)width,
		(uint32_t)height,
		uMsaaCount);

	m_DepthStencil = false;

	return true;
}

bool KVulkanRenderTarget::UnInit()
{
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	if (m_FrameBuffer)
	{
		((KVulkanFrameBuffer*)m_FrameBuffer.get())->UnInit();
		m_FrameBuffer = nullptr;
	}

	return true;
}

bool KVulkanRenderTarget::IsDepthStencil()
{
	return m_DepthStencil;
}

bool KVulkanRenderTarget::GetSize(size_t& width, size_t& height)
{
	if (m_FrameBuffer)
	{
		width = m_FrameBuffer->GetWidth();
		height = m_FrameBuffer->GetHeight();
		return true;
	}

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
	if (m_FrameBuffer)
	{
		return ((KVulkanFrameBuffer*)m_FrameBuffer.get())->GetMSAAFlag();
	}

	assert(false && "should not reach");
	return VK_SAMPLE_COUNT_1_BIT;
}