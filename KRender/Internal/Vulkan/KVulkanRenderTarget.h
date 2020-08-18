#pragma once
#include "Interface/IKRenderTarget.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanRenderTarget : public IKRenderTarget
{
protected:
	IKFrameBufferPtr m_ColorFrameBuffer;
	IKFrameBufferPtr m_DepthFrameBuffer;
	IKRenderPassPtr m_RenderPass;

	KVulkanRenderTarget();
public:
	static IKRenderTargetPtr CreateRenderTarget();
public:
	~KVulkanRenderTarget();

	virtual bool InitFromSwapChain(IKSwapChain* swapChain, size_t imageIndex, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	virtual bool InitFromTexture(IKTexture* texture, bool bDepth, bool bStencil, unsigned short uMsaaCount);

	virtual bool InitFromDepthStencil(size_t width, size_t height, bool bStencil);
	virtual bool InitFromColor(size_t width, size_t height, unsigned short uMsaaCount, ElementFormat format);
	virtual bool UnInit();

	virtual bool GetSize(size_t& width, size_t& height);

	virtual bool HasColorAttachment();
	virtual bool HasDepthStencilAttachment();

	VkRenderPass GetRenderPass();
	VkFramebuffer GetFrameBuffer();
	VkExtent2D GetExtend();
	VkSampleCountFlagBits GetMsaaFlag();

	bool GetImageViewInformation(RenderTargetComponent component, VkFormat& format, VkImageView& imageView);
};