#pragma once
#include "KRender/Interface/IKRenderDevice.h"

struct IKFrameBuffer
{
	virtual ~IKFrameBuffer() {}
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual uint32_t GetDepth() const = 0;
	virtual uint32_t GetMipmaps() const = 0;
	virtual uint32_t GetMSAA() const = 0;
	virtual bool IsDepthStencil() const = 0;
	virtual bool IsStroageImage() const = 0;

	virtual bool TranslateToStorage(IKCommandBufferPtr commandBuffer = nullptr) = 0;
	virtual bool TranslateToShader(IKCommandBufferPtr commandBuffer = nullptr) = 0;
};