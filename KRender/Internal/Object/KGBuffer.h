#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Publish/KCamera.h"

class KGBuffer
{
protected:
	IKRenderTargetPtr m_RenderTarget;
	IKRenderTargetPtr m_DepthStencilTarget;
	IKRenderPassPtr m_RenderPass;
	IKSamplerPtr m_GBufferSampler;
	std::vector<IKCommandBufferPtr> m_CommandBuffers;
	IKCommandPoolPtr m_CommandPool;
	KRenderStageStatistics m_Statistics;
	IKRenderDevice* m_RenderDevice;
	const KCamera* m_Camera;
public:
	KGBuffer();
	~KGBuffer();

	bool Init(IKRenderDevice* renderDevice, const KCamera* camera, uint32_t width, uint32_t height, size_t frameInFlight);
	bool UnInit();

	bool Resize(uint32_t width, uint32_t height);
	bool UpdateGBuffer(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);

	inline IKRenderTargetPtr GetGBufferTarget() { return m_RenderTarget; }
	inline IKSamplerPtr GetSampler() { return m_GBufferSampler; }
};