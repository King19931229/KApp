#pragma once
#include "KHiZBuffer.h"
#include "Internal/Object/KDebugDrawer.h"

class KScreenSpaceReflection
{
protected:
	IKRenderTargetPtr m_HitResultTarget;
	IKRenderTargetPtr m_HitMaskTarget;
	IKRenderTargetPtr m_TemporalTarget[2];
	IKRenderTargetPtr m_FinalTarget;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_CurrentIdx;
	float m_Ratio;
	bool m_FirstFrame;

	KShaderRef m_QuadVS;
	KShaderRef m_ReflectionFS;
	KShaderRef m_RayReuseFS;
	KShaderRef m_TemporalFS;
	KShaderRef m_BlitFS;

	IKPipelinePtr m_ReflectionPipeline;
	IKPipelinePtr m_RayReusePipeline;
	IKPipelinePtr m_TemporalPipeline[2];
	IKPipelinePtr m_BlitPipeline;
	IKRenderPassPtr m_ReflectionPass;
	IKRenderPassPtr m_RayReusePass[2];
	IKRenderPassPtr m_BlitPass[2];
	IKRenderPassPtr m_TemporalPass;

	IKCommandBufferPtr m_PrimaryCommandBuffer;
	IKCommandPoolPtr m_CommandPool;

	KRTDebugDrawer m_DebugDrawer;

	void InitializePipeline();
public:
	KScreenSpaceReflection();
	~KScreenSpaceReflection();

	bool Init(uint32_t width, uint32_t height, float ratio);
	bool UnInit();

	bool& GetDebugDrawEnable() { return m_DebugDrawer.GetEnable(); }

	bool ReloadShader();
	bool Resize(uint32_t width, uint32_t height);

	bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);
	bool Execute(IKCommandBufferPtr primaryBuffer);
};