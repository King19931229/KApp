#pragma once
#include "IKRenderConfig.h"

struct IKSwapChain
{
	virtual ~IKSwapChain() {}
	virtual bool Init(IKRenderWindow* window, uint32_t frameInFlight) = 0;
	virtual bool UnInit() = 0;
	virtual IKRenderWindow* GetWindow() = 0;
	virtual uint32_t GetFrameInFlight() = 0;
	virtual uint32_t GetWidth() = 0;
	virtual uint32_t GetHeight() = 0;

	virtual IKRenderPassPtr GetRenderPass(uint32_t chainIndex) = 0;
	virtual IKFrameBufferPtr GetColorFrameBuffer(uint32_t chainIndex) = 0;
	virtual IKFrameBufferPtr GetDepthStencilFrameBuffer(uint32_t chainIndex) = 0;
};