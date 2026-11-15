#pragma once
#include "Interface/IKSwapChain.h"
#include <algorithm>
#include "KBase/Publish/KTemplate.h"

class KHeadlessSwapChain : public IKSwapChain
{
protected:
	struct Frame
	{
		IKRenderTargetPtr color;
		IKRenderTargetPtr depth;
		IKRenderPassPtr renderPass;
		bool submitted = false;
	};
	IKRenderWindow* m_Window = nullptr;
	uint32_t m_Width = 0, m_Height = 0, m_CurrentImageIndex = 0;
	std::vector<Frame> m_Frames;
	IKQueuePtr m_Queue;
	IKSemaphorePtr m_ImageAvailable;
	IKSemaphorePtr m_RenderFinished;
	IKFencePtr m_InFlightFence;
	std::vector<KDevicePresentCallback*> m_PrePresentCallbacks;
	std::vector<KDevicePresentCallback*> m_PostPresentCallbacks;
	std::vector<KSwapChainRecreateCallback*> m_RecreateCallbacks;

public:
	~KHeadlessSwapChain() override;
	bool Init(IKRenderWindow* window) override;
	bool UnInit() override;
	IKRenderWindow* GetWindow() override { return m_Window; }
	uint32_t GetFrameInFlight() override { return (uint32_t)m_Frames.size(); }
	uint32_t GetWidth() override { return m_Width; }
	uint32_t GetHeight() override { return m_Height; }

	bool WaitForInFlightFrame(uint32_t& completedFrameNum) override;
	SwapChainAcquireResult AcquireNextImage(uint32_t& imageIndex) override;
	void PresentQueue(bool& needResize) override;
	IKSemaphorePtr GetImageAvailableSemaphore() override { return m_ImageAvailable; }
	IKSemaphorePtr GetRenderFinishSemaphore() override { return m_RenderFinished; }
	IKFencePtr GetInFlightFence() override { return m_InFlightFence; }
	IKRenderPassPtr GetRenderPass(uint32_t chainIndex) override;
	IKFrameBufferPtr GetColorFrameBuffer(uint32_t chainIndex) override;
	IKFrameBufferPtr GetDepthStencilFrameBuffer(uint32_t chainIndex) override;

	bool RegisterPrePresentCallback(KDevicePresentCallback* callback) override { return KTemplate::RegisterCallback(m_PrePresentCallbacks, callback); }
	bool UnRegisterPrePresentCallback(KDevicePresentCallback* callback) override { return KTemplate::UnRegisterCallback(m_PrePresentCallbacks, callback); }
	bool RegisterPostPresentCallback(KDevicePresentCallback* callback) override { return KTemplate::RegisterCallback(m_PostPresentCallbacks, callback); }
	bool UnRegisterPostPresentCallback(KDevicePresentCallback* callback) override { return KTemplate::UnRegisterCallback(m_PostPresentCallbacks, callback); }
	bool RegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback) override { return KTemplate::RegisterCallback(m_RecreateCallbacks, callback); }
	bool UnRegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback) override { return KTemplate::UnRegisterCallback(m_RecreateCallbacks, callback); }
};
