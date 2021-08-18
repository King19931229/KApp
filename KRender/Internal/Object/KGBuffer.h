#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Publish/KCamera.h"

class KGBuffer
{
protected:
	enum
	{
		RT_COUNT = 3
	};
	IKRenderTargetPtr m_RenderTarget[RT_COUNT];
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

	inline IKRenderTargetPtr GetGBufferTarget0() { return m_RenderTarget[0]; }
	inline IKRenderTargetPtr GetGBufferTarget1() { return m_RenderTarget[1]; }
	inline IKRenderTargetPtr GetGBufferTarget2() { return m_RenderTarget[2]; }
	inline IKSamplerPtr GetSampler() { return m_GBufferSampler; }
};