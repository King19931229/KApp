#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Publish/KCamera.h"

class KGBuffer
{
public:
	enum RTType
	{
		RT_NORMAL,
		RT_POSITION,
		RT_MOTION,
		RT_DIFFUSE,
		RT_SPECULAR,
		RT_COUNT
	};
protected:
	IKRenderTargetPtr m_RenderTarget[RT_COUNT];
	IKRenderTargetPtr m_DepthStencilTarget;
	IKRenderPassPtr m_PreZPass;
	IKRenderPassPtr m_MainPass;
	IKSamplerPtr m_GBufferSampler;

	enum GBufferStage
	{
		GBUFFER_STAGE_PRE_Z,
		GBUFFER_STAGE_DEFAULT,
		GBUFFER_STAGE_COUNT
	};

	IKCommandBufferPtr m_CommandBuffers[GBUFFER_STAGE_COUNT];
	KRenderStageStatistics m_Statistics[GBUFFER_STAGE_COUNT];

	IKCommandPoolPtr m_CommandPool;
	IKRenderDevice* m_RenderDevice;
	const KCamera* m_Camera;
public:
	KGBuffer();
	~KGBuffer();

	bool Init(IKRenderDevice* renderDevice, const KCamera* camera, uint32_t width, uint32_t height);
	bool UnInit();

	bool Resize(uint32_t width, uint32_t height);
	bool UpdatePreDepth(IKCommandBufferPtr primaryBuffer);
	bool UpdateGBuffer(IKCommandBufferPtr primaryBuffer);

	inline IKRenderTargetPtr GetGBufferTarget(RTType rt) { return m_RenderTarget[rt]; }
	inline IKRenderTargetPtr GetDepthStencilTarget() { return m_DepthStencilTarget; }
	inline IKSamplerPtr GetSampler() { return m_GBufferSampler; }
};