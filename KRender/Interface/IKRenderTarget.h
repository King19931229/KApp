#pragma once
#include "IKRenderConfig.h"

struct IKRenderTarget
{
	virtual ~IKRenderTarget() {}

	virtual bool SetSize(size_t width, size_t height) = 0;

	virtual bool SetColorClear(float r, float g, float b, float a) = 0;
	virtual bool SetDepthStencilClear(float depth, unsigned int stencil) = 0;

	virtual bool InitFromImage(void* imageHandle, void* imageFormatHandle,
		bool bDepth,
		bool bStencil,
		unsigned short uMsaaCount) = 0;
	virtual bool UnInit() = 0;
};