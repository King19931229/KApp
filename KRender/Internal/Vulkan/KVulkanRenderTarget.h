#pragma once
#include "Interface/IKRenderTarget.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanRenderTarget : public IKRenderTarget
{
protected:
	IKFrameBufferPtr m_FrameBuffer;
	bool m_DepthStencil;
public:
	KVulkanRenderTarget();
	~KVulkanRenderTarget();

	virtual bool InitFromDepthStencil(uint32_t width, uint32_t height, uint32_t msaaCount, bool bStencil);
	virtual bool InitFromColor(uint32_t width, uint32_t height, uint32_t msaaCount, ElementFormat format);
	virtual bool UnInit();
	virtual bool IsDepthStencil();

	virtual bool GetSize(size_t& width, size_t& height);
	virtual IKFrameBufferPtr GetFrameBuffer();

	VkExtent2D GetExtend();
	VkSampleCountFlagBits GetMsaaFlag();
};