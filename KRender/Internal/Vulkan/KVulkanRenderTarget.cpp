#include "KVulkanRenderTarget.h"
#include "KVulkanSwapChain.h"
#include "KVulkanTexture.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanRenderPass.h"
#include "Internal/KRenderGlobal.h"

IKRenderTargetPtr KVulkanRenderTarget::CreateRenderTarget()
{
	return IKRenderTargetPtr(KNEW KVulkanRenderTarget());
}

KVulkanRenderTarget::KVulkanRenderTarget()
{
}

KVulkanRenderTarget::~KVulkanRenderTarget()
{
	ASSERT_RESULT(m_RenderPass == nullptr);
	ASSERT_RESULT(m_ColorFrameBuffer == nullptr);
	ASSERT_RESULT(m_DepthFrameBuffer == nullptr);
}

bool KVulkanRenderTarget::InitFromSwapChain(IKSwapChain* swapChain, size_t imageIndex, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	UnInit();

	KVulkanSwapChain* vulkanSwapChain = (KVulkanSwapChain*)swapChain;

	m_ColorFrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());

	((KVulkanFrameBuffer*)m_ColorFrameBuffer.get())->InitExternal(
		vulkanSwapChain->GetImage(imageIndex),
		vulkanSwapChain->GetImageView(imageIndex),
		vulkanSwapChain->GetImageFormat(),
		vulkanSwapChain->GetWidth(),
		vulkanSwapChain->GetHeight(),
		1,1,uMsaaCount);
	
	m_DepthFrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
	((KVulkanFrameBuffer*)m_DepthFrameBuffer.get())->InitDepthStencil(m_ColorFrameBuffer->GetWidth(), m_ColorFrameBuffer->GetHeight(), uMsaaCount, bStencil);

	m_RenderPass = IKRenderPassPtr(KNEW KVulkanRenderPass());
	m_RenderPass->SetColor(0, m_ColorFrameBuffer);
	m_RenderPass->SetDepthStencil(m_DepthFrameBuffer);
	m_RenderPass->SetAsSwapChainPass(true);
	m_RenderPass->Init();

	return true;
}

bool KVulkanRenderTarget::InitFromTexture(IKTexture* texture, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	UnInit();

	KVulkanTexture* vulkanTexture = (KVulkanTexture*)texture;

	m_ColorFrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
	/*
	((KVulkanFrameBuffer*)m_ColorFrameBuffer.get())->InitColor(
		vulkanTexture->GetImageFormat(),
		vulkanTexture->GetTextureType(),
		(uint32_t)vulkanTexture->GetWidth(),
		(uint32_t)vulkanTexture->GetHeight(),
		uMsaaCount);
	*/

	((KVulkanFrameBuffer*)m_ColorFrameBuffer.get())->InitExternal(
		vulkanTexture->GetImage(),
		vulkanTexture->GetImageView(),
		vulkanTexture->GetImageFormat(),
		(uint32_t)vulkanTexture->GetWidth(),
		(uint32_t)vulkanTexture->GetHeight(),
		1, 1, uMsaaCount);

	if (bDepth)
	{
		m_DepthFrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
		((KVulkanFrameBuffer*)m_DepthFrameBuffer.get())->InitDepthStencil(m_ColorFrameBuffer->GetWidth(), m_ColorFrameBuffer->GetHeight(), uMsaaCount, bStencil);
	}

	m_RenderPass = IKRenderPassPtr(KNEW KVulkanRenderPass());
	m_RenderPass->SetColor(0, m_ColorFrameBuffer);
	if (bDepth)
		m_RenderPass->SetDepthStencil(m_DepthFrameBuffer);
	m_RenderPass->Init();

	return true;
}

bool KVulkanRenderTarget::InitFromDepthStencil(size_t width, size_t height, bool bStencil)
{
	UnInit();

	m_DepthFrameBuffer = IKFrameBufferPtr(KNEW KVulkanFrameBuffer());
	((KVulkanFrameBuffer*)m_DepthFrameBuffer.get())->InitDepthStencil((uint32_t)width, (uint32_t)height, 1, bStencil);

	m_RenderPass = IKRenderPassPtr(KNEW KVulkanRenderPass());
	m_RenderPass->SetDepthStencil(m_DepthFrameBuffer);
	m_RenderPass->Init();

	return true;
}

bool KVulkanRenderTarget::UnInit()
{
	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	if (m_RenderPass)
	{
		m_RenderPass->UnInit();
		m_RenderPass = nullptr;
	}

	if (m_ColorFrameBuffer)
	{
		((KVulkanFrameBuffer*)m_ColorFrameBuffer.get())->UnInit();
		m_ColorFrameBuffer = nullptr;
	}

	if (m_DepthFrameBuffer)
	{
		((KVulkanFrameBuffer*)m_DepthFrameBuffer.get())->UnInit();
		m_DepthFrameBuffer = nullptr;
	}

	KRenderGlobal::PipelineManager.InvaildateHandleByRt(shared_from_this());

	return true;
}

bool KVulkanRenderTarget::GetSize(size_t& width, size_t& height)
{
	if (m_ColorFrameBuffer)
	{
		width = m_ColorFrameBuffer->GetWidth();
		height = m_ColorFrameBuffer->GetHeight();
		return true;
	}

	if (m_DepthFrameBuffer)
	{
		width = m_DepthFrameBuffer->GetWidth();
		height = m_DepthFrameBuffer->GetHeight();
		return true;
	}
	
	return true;
}

VkRenderPass KVulkanRenderTarget::GetRenderPass()
{
	ASSERT_RESULT(m_RenderPass);
	return ((KVulkanRenderPass*)m_RenderPass.get())->GetVkRenderPass();
}

VkFramebuffer KVulkanRenderTarget::GetFrameBuffer()
{
	ASSERT_RESULT(m_RenderPass);
	return ((KVulkanRenderPass*)m_RenderPass.get())->GetVkFrameBuffer(0);
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
	if (m_ColorFrameBuffer)
	{
		return ((KVulkanFrameBuffer*)m_ColorFrameBuffer.get())->GetMSAAFlag();
	}

	if (m_DepthFrameBuffer)
	{
		return ((KVulkanFrameBuffer*)m_DepthFrameBuffer.get())->GetMSAAFlag();
	}

	assert(false && "should not reach");
	return VK_SAMPLE_COUNT_1_BIT;
}

bool KVulkanRenderTarget::HasColorAttachment()
{
	if (m_RenderPass && m_RenderPass->HasColorAttachment())
		return true;
	return false;
}

bool KVulkanRenderTarget::HasDepthStencilAttachment()
{
	if (m_RenderPass && m_RenderPass->HasDepthStencilAttachment())
		return true;
	return false;
}

bool KVulkanRenderTarget::GetImageViewInformation(RenderTargetComponent component, VkFormat& format, VkImageView& imageView)
{
	switch (component)
	{
	case RTC_COLOR:
		if(m_ColorFrameBuffer)
		{
			format = ((KVulkanFrameBuffer*)m_ColorFrameBuffer.get())->GetForamt();
			imageView = ((KVulkanFrameBuffer*)m_ColorFrameBuffer.get())->GetImageView();
			return true;
		}
		else
		{
			return false;
		}
	case RTC_DEPTH_STENCIL:		
		if (m_DepthFrameBuffer)
		{
			format = ((KVulkanFrameBuffer*)m_DepthFrameBuffer.get())->GetForamt();
			imageView = ((KVulkanFrameBuffer*)m_DepthFrameBuffer.get())->GetImageView();
			return true;
		}
		else
		{
			return false;
		}
	default:
		assert(false && "unknown component");
		return false;
	}
}