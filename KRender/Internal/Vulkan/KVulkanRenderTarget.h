#pragma once
#include "Interface/IKRenderTarget.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanRenderTarget : public IKRenderTarget
{
protected:
	VkRenderPass	m_RenderPass;
	VkFramebuffer	m_FrameBuffer;

	VkExtent2D		m_Extend;

	VkImageView		m_ColorImageView;
	VkFormat		m_ColorFormat;

	VkFormat		m_DepthFormat;

	VkImage			m_DepthImage;
	VkImageView		m_DepthImageView;
	KVulkanHeapAllocator::AllocInfo m_DepthAlloc;

	VkSampleCountFlagBits m_MsaaFlag;
	VkImage			m_MsaaImage;
	VkImageView		m_MsaaImageView;
	KVulkanHeapAllocator::AllocInfo m_MsaaAlloc;

	static VkFormat FindDepthFormat(bool bStencil);

	bool CreateImage(VkImageView imageView, VkFormat imageForamt,bool bDepth, bool bStencil, unsigned short uMsaaCount);
	bool CreateFramebuffer(bool fromSwapChain);

	bool CreateDepthImage(bool bStencil);
	bool CreateDepthBuffer();

	KVulkanRenderTarget();
public:
	static IKRenderTargetPtr CreateRenderTarget();
public:
	~KVulkanRenderTarget();

	virtual bool InitFromSwapChain(IKSwapChain* swapChain, size_t imageIndex, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	virtual bool InitFromTexture(IKTexture* texture, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	virtual bool InitFromDepthStencil(size_t width, size_t height, bool bStencil);

	virtual bool UnInit();
	virtual bool GetSize(size_t& width, size_t& height);

	virtual bool HasColorAttachment();
	virtual bool HasDepthStencilAttachment();

	inline VkRenderPass GetRenderPass() { return m_RenderPass; }
	inline VkFramebuffer GetFrameBuffer() { return m_FrameBuffer; }
	inline VkSampleCountFlagBits GetMsaaFlag(){ return m_MsaaFlag; }
	inline VkExtent2D GetExtend() { return m_Extend; }

	bool GetImageViewInformation(RenderTargetComponent component, VkFormat& format, VkImageView& imageView);
};