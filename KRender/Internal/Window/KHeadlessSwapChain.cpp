#include "KHeadlessSwapChain.h"
#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKRenderPass.h"
#include "Interface/IKQueue.h"
#include "Internal/KRenderGlobal.h"

KHeadlessSwapChain::~KHeadlessSwapChain()
{
	ASSERT_RESULT(m_Frames.empty());
}

bool KHeadlessSwapChain::Init(IKRenderWindow* window)
{
	UnInit();
	size_t width = 0, height = 0;
	if (!window || !window->GetSize(width, height) || !width || !height)
	{
		return false;
	}
	m_Window = window;
	m_Width = (uint32_t)width;
	m_Height = (uint32_t)height;
	IKRenderDevice* device = KRenderGlobal::RenderDevice;
	device->CreateQueue(m_Queue);
	device->CreateSemaphore(m_ImageAvailable);
	device->CreateSemaphore(m_RenderFinished);
	device->CreateFence(m_InFlightFence);
	if (!m_Queue->Init(QUEUE_GRAPHICS, 0) || !m_ImageAvailable->Init() ||
		!m_RenderFinished->Init() || !m_InFlightFence->Init(true))
	{
		UnInit();
		return false;
	}
	m_ImageAvailable->SetDebugName("HeadlessImageAvailable");
	m_RenderFinished->SetDebugName("HeadlessRenderFinished");
	m_InFlightFence->SetDebugName("HeadlessInFlightFence");
	m_Frames.resize(KRenderGlobal::NumFramesInFlight);
	for (Frame& frame : m_Frames)
	{
		device->CreateRenderTarget(frame.color);
		device->CreateRenderTarget(frame.depth);
		device->CreateRenderPass(frame.renderPass);
		if (!frame.color->InitFromColor(m_Width, m_Height, 1, 1, PF_R8G8B8A8_UNORM) ||
			!frame.depth->InitFromDepthStencil(m_Width, m_Height, 1, true))
		{
			UnInit();
			return false;
		}
		frame.renderPass->SetAsSwapChainPass(false);
		frame.renderPass->SetColorAttachment(0, frame.color->GetFrameBuffer());
		frame.renderPass->SetClearColor(0, { 0, 0, 0, 0 });
		frame.renderPass->SetDepthStencilAttachment(frame.depth->GetFrameBuffer());
		frame.renderPass->SetClearDepthStencil({ 1, 0 });
		if (!frame.renderPass->Init())
		{
			UnInit();
			return false;
		}
	}
	for (auto callback : m_RecreateCallbacks) (*callback)(m_Width, m_Height);
	return true;
}

bool KHeadlessSwapChain::UnInit()
{
	if (m_Window) KRenderGlobal::RenderDevice->Wait();
	for (Frame& frame : m_Frames)
	{
		if (frame.renderPass) frame.renderPass->UnInit();
		if (frame.color) frame.color->UnInit();
		if (frame.depth) frame.depth->UnInit();
	}
	m_Frames.clear();
	if (m_InFlightFence) m_InFlightFence->UnInit();
	if (m_ImageAvailable) m_ImageAvailable->UnInit();
	if (m_RenderFinished) m_RenderFinished->UnInit();
	if (m_Queue) m_Queue->UnInit();
	m_InFlightFence = nullptr;
	m_ImageAvailable = nullptr;
	m_RenderFinished = nullptr;
	m_Queue = nullptr;
	m_Window = nullptr;
	m_Width = m_Height = m_CurrentImageIndex = 0;
	// Device::RecreateSwapChain uses UnInit/Init on the same object.
	return true;
}

void KHeadlessSwapChain::WaitForInFlightFrame()
{
	ASSERT_RESULT(m_InFlightFence->Wait());
}

SwapChainAcquireResult KHeadlessSwapChain::AcquireNextImage(uint32_t& imageIndex)
{
	size_t width = 0, height = 0;
	if (!m_Window || !m_Window->GetSize(width, height)) return SCAR_FAILED;
	if (width != m_Width || height != m_Height) return SCAR_OUT_OF_DATE;
	const uint32_t slot = KRenderGlobal::CurrentInFlightFrameIndex;
	std::vector<KSemaphoreWaitInfo> waits;
	if (m_Frames[slot].submitted)
	{
		// Consume the previous render's binary semaphore before signaling it again.
		waits.push_back({ m_RenderFinished, PIPELINE_STAGE_ALL_COMMANDS });
	}
	if (!m_Queue->Submit(nullptr, waits, { m_ImageAvailable }, nullptr)) return SCAR_FAILED;
	imageIndex = m_CurrentImageIndex = slot;
	return SCAR_SUCCESS;
}

void KHeadlessSwapChain::PresentQueue(bool& needResize)
{
	const uint32_t slot = KRenderGlobal::CurrentInFlightFrameIndex;
	for (auto callback : m_PrePresentCallbacks) (*callback)(m_CurrentImageIndex, slot);
	m_Frames[slot].submitted = true;
	needResize = false;
	for (auto callback : m_PostPresentCallbacks) (*callback)(m_CurrentImageIndex, slot);
}

IKRenderPassPtr KHeadlessSwapChain::GetRenderPass(uint32_t chainIndex)
{
	return chainIndex < m_Frames.size() ? m_Frames[chainIndex].renderPass : nullptr;
}

IKFrameBufferPtr KHeadlessSwapChain::GetColorFrameBuffer(uint32_t chainIndex)
{
	return chainIndex < m_Frames.size() ? m_Frames[chainIndex].color->GetFrameBuffer() : nullptr;
}

IKFrameBufferPtr KHeadlessSwapChain::GetDepthStencilFrameBuffer(uint32_t chainIndex)
{
	return chainIndex < m_Frames.size() ? m_Frames[chainIndex].depth->GetFrameBuffer() : nullptr;
}
