#pragma once
#include "KRender/Interface/IKRenderDevice.h"
#include "KBase/Interface/IKCodec.h"

struct IKFrameBuffer
{
	virtual ~IKFrameBuffer() {}
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual uint32_t GetDepth() const = 0;
	virtual uint32_t GetMipmaps() const = 0;
	virtual uint32_t GetMSAA() const = 0;
};

struct IKRenderPass
{
	virtual ~IKRenderPass() {}
	virtual bool SetColor(uint32_t attachment, IKFrameBufferPtr color) = 0;
	virtual bool SetDepthStencil(IKFrameBufferPtr depthStencil) = 0;
	virtual bool SetAsSwapChainPass(bool swapChain) = 0;
	virtual bool HasColorAttachment() = 0;
	virtual bool HasDepthStencilAttachment() = 0;
	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
};