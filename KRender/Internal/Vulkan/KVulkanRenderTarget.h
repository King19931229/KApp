#pragma once
#include "Interface/IKRenderTarget.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanRenderTarget : public IKRenderTarget
{
protected:
	IKFrameBufferPtr m_FrameBuffer;
public:
	KVulkanRenderTarget();
	~KVulkanRenderTarget();

	virtual bool InitFromDepthStencil(uint32_t width, uint32_t height, uint32_t msaaCount, bool bStencil);
	virtual bool InitFromColor(uint32_t width, uint32_t height, uint32_t msaaCount, uint32_t mipmaps, ElementFormat format);
	virtual bool InitFromStorage(uint32_t width, uint32_t height, uint32_t mipmaps, ElementFormat format);
	virtual bool InitFromStorage3D(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, ElementFormat format);
	virtual bool InitFromReadback(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, ElementFormat format);
	virtual bool UnInit();

	virtual bool GetSize(size_t& width, size_t& height);
	virtual IKFrameBufferPtr GetFrameBuffer();

	VkExtent2D GetExtend();
	VkSampleCountFlagBits GetMsaaFlag();
};