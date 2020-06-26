#pragma once
#include "IKRenderConfig.h"

struct IKSwapChain
{
	virtual ~IKSwapChain() {}
	virtual bool Init(IKRenderWindow* window, uint32_t frameInFlight) = 0;
	virtual bool UnInit() = 0;
	virtual uint32_t GetWidth() = 0;
	virtual uint32_t GetHeight() = 0;
	virtual IKRenderTargetPtr GetRenderTarget(uint32_t frameIndex) = 0;
};