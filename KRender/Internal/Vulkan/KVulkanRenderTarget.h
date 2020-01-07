#pragma once
#include "Interface/IKRenderTarget.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanRenderTarget : public IKRenderTarget
{
protected:
	VkRenderPass	m_RenderPass;
	VkFramebuffer	m_FrameBuffer;

	enum ClearTarget
	{
		CT_COLOR,
		CT_DEPTH_STENCIL,

		CT_COUNT
	};
	VkClearValue	m_ClearValues[CT_COUNT];

	VkExtent2D		m_Extend;

	VkImageView		m_ColorImageView;
	VkFormat		m_ColorFormat;

	VkFormat		m_DepthFormat;

	VkImage			m_DepthImage;
	VkImageView		m_DepthImageView;
	KVulkanHeapAllocator::AllocInfo m_DepthAlloc;

	VkImage			m_DepthResolveImage;
	VkImageView		m_DepthResolveImageView;
	KVulkanHeapAllocator::AllocInfo m_DepthResolveAlloc;

	VkSampleCountFlagBits m_MsaaFlag;
	VkImage			m_MsaaImage;
	VkImageView		m_MsaaImageView;
	KVulkanHeapAllocator::AllocInfo m_MsaaAlloc;

	bool			m_bMsaaCreated;
	bool			m_bDepthStencilCreated;

	static VkFormat FindDepthFormat(bool bStencil);

	bool CreateImage(VkImageView imageView, VkFormat imageForamt,bool bDepth, bool bStencil, unsigned short uMsaaCount);
	bool CreateFramebuffer(bool fromSwapChain);

	bool CreateDepthImage(bool bStencil);
	bool CreateDepthBuffer();
public:
	KVulkanRenderTarget();
	~KVulkanRenderTarget();

	virtual bool SetColorClear(float r, float g, float b, float a);
	virtual bool SetDepthStencilClear(float depth, unsigned int stencil);

	virtual bool InitFromSwapChain(IKSwapChain* swapChain, size_t imageIndex, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	virtual bool InitFromTexture(IKTexture* texture, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	virtual bool InitFromDepthStencil(size_t width, size_t height, bool bStencil);

	virtual bool UnInit();
	virtual bool GetSize(size_t& width, size_t& height);

	inline VkRenderPass GetRenderPass() { return m_RenderPass; }
	inline VkFramebuffer GetFrameBuffer() { return m_FrameBuffer; }
	inline VkSampleCountFlagBits GetMsaaFlag(){ return m_MsaaFlag; }
	inline VkExtent2D GetExtend() { return m_Extend; }

	typedef std::pair<VkClearValue*, unsigned int> ClearValues;
	ClearValues GetVkClearValues();
	bool GetImageViewInformation(RenderTargetComponent component, VkFormat& format, VkImageView& imageView);
};