#pragma once
#include "IKRenderConfig.h"

struct IKRenderTarget : public std::enable_shared_from_this<IKRenderTarget>
{
	virtual ~IKRenderTarget() {}

	virtual bool InitFromSwapChain(IKSwapChain* swapChain, size_t imageIndex, bool bDepth, bool bStencil, unsigned short uMsaaCount) = 0;
	virtual bool InitFromTexture(IKTexture* texture, bool bDepth, bool bStencil, unsigned short uMsaaCount) = 0;

	virtual bool InitFromDepthStencil(size_t width, size_t height, bool bStencil) = 0;
	virtual bool InitFromColor(size_t width, size_t height, unsigned short uMsaaCount, ElementFormat format) = 0;
	virtual bool UnInit() = 0;

	virtual bool GetSize(size_t& width, size_t& height) = 0;

	virtual bool HasColorAttachment() = 0;
	virtual bool HasDepthStencilAttachment() = 0;
};