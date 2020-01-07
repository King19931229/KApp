#pragma once
#include "IKRenderConfig.h"

struct IKSwapChain
{
	virtual ~IKSwapChain() {}
	virtual bool Init(uint32_t width, uint32_t height, size_t frameInFlight) = 0;
	virtual bool UnInit() = 0;
	virtual uint32_t GetWidth() = 0;
	virtual uint32_t GetHeight() = 0;
	virtual IKRenderTarget* GetRenderTarget(size_t frameIndex) = 0;
};