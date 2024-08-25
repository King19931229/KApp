#pragma once
#include "IKRenderConfig.h"
#include <functional>

typedef std::function<void(uint32_t chainIndex, uint32_t frameIndex)> KDevicePresentCallback;
typedef std::function<void(uint32_t width, uint32_t height)> KSwapChainRecreateCallback;

struct IKSwapChain
{
	virtual ~IKSwapChain() {}
	virtual bool Init(IKRenderWindow* window, uint32_t frameInFlight) = 0;
	virtual bool UnInit() = 0;
	virtual IKRenderWindow* GetWindow() = 0;
	virtual uint32_t GetFrameInFlight() = 0;
	virtual uint32_t GetWidth() = 0;
	virtual uint32_t GetHeight() = 0;

	virtual void WaitForInFlightFrame(uint32_t& frameIndex) = 0;
	virtual void AcquireNextImage(uint32_t& imageIndex) = 0;
	virtual void PresentQueue(bool& needResize) = 0;

	virtual IKSemaphorePtr GetImageAvailableSemaphore() = 0;
	virtual IKSemaphorePtr GetRenderFinishSemaphore() = 0;
	virtual IKFencePtr GetInFlightFence() = 0;

	virtual IKRenderPassPtr GetRenderPass(uint32_t chainIndex) = 0;
	virtual IKFrameBufferPtr GetColorFrameBuffer(uint32_t chainIndex) = 0;
	virtual IKFrameBufferPtr GetDepthStencilFrameBuffer(uint32_t chainIndex) = 0;

	virtual bool RegisterPrePresentCallback(KDevicePresentCallback* callback) = 0;
	virtual bool UnRegisterPrePresentCallback(KDevicePresentCallback* callback) = 0;
	virtual bool RegisterPostPresentCallback(KDevicePresentCallback* callback) = 0;
	virtual bool UnRegisterPostPresentCallback(KDevicePresentCallback* callback) = 0;

	virtual bool RegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback) = 0;
	virtual bool UnRegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback) = 0;
};