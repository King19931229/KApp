#pragma once
#include "IKRenderConfig.h"

struct IKRenderTarget
{
	virtual ~IKRenderTarget() {}

	virtual bool InitFromDepthStencil(uint32_t width, uint32_t height, uint32_t msaaCount, bool bStencil) = 0;
	virtual bool InitFromColor(uint32_t width, uint32_t height, uint32_t msaaCount, ElementFormat format) = 0;
	virtual bool InitFromStorage(uint32_t width, uint32_t height, uint32_t depth, ElementFormat format) = 0;
	virtual bool UnInit() = 0;
	virtual bool IsDepthStencil() = 0;
	virtual IKFrameBufferPtr GetFrameBuffer() = 0;
	virtual bool GetSize(size_t& width, size_t& height) = 0;
};