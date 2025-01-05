#pragma once
#include "KHiZBuffer.h"
#include "Internal/Object/KDebugDrawer.h"

class KScreenSpaceReflection
{
protected:
	IKRenderTargetPtr m_HitResultTarget;
	IKRenderTargetPtr m_HitMaskTarget;
	IKRenderTargetPtr m_TemporalTarget[2];
	IKRenderTargetPtr m_TemporalSquaredTarget[2];
	IKRenderTargetPtr m_TemporalTsppTarget[2];
	IKRenderTargetPtr m_BlurTarget[2];
	IKRenderTargetPtr m_FinalTarget;
	IKRenderTargetPtr m_FinalSquaredTarget;
	IKRenderTargetPtr m_FinalTsppTarget;
	IKRenderTargetPtr m_VarianceTarget;
	IKRenderTargetPtr m_ComposeTarget;
	uint32_t m_FullWidth;
	uint32_t m_FullHeight;
	uint32_t m_Width;
	uint32_t m_Height;
	int32_t m_RayReuseCount;
	int32_t m_AtrousLevel;
	uint32_t m_CurrentIdx;
	float m_Ratio;
	bool m_Enable;
	bool m_ResolveInFullResolution;
	bool m_FirstFrame;

	KShaderRef m_QuadVS;
	KShaderRef m_ReflectionFS;
	KShaderRef m_RayReuseFS;
	KShaderRef m_TemporalFS;
	KShaderRef m_BlitFS;
	KShaderRef m_AtrousFS;
	KShaderRef m_ComposeFS;

	IKPipelinePtr m_ReflectionPipeline;
	IKPipelinePtr m_RayReusePipeline;
	IKPipelinePtr m_TemporalPipeline[2];
	IKPipelinePtr m_AtrousPipeline[2];
	IKPipelinePtr m_ComposePipeline[2];
	IKPipelinePtr m_BlitPipeline;
	IKRenderPassPtr m_ReflectionPass;
	IKRenderPassPtr m_RayReusePass[2];
	IKRenderPassPtr m_BlitPass[2];
	IKRenderPassPtr m_TemporalPass;
	IKRenderPassPtr m_AtrousPass[2];
	IKRenderPassPtr m_ComposePass;

	KRTDebugDrawer m_DebugDrawer;

	void InitializePipeline();
public:
	KScreenSpaceReflection();
	~KScreenSpaceReflection();

	bool Init(uint32_t width, uint32_t height, float ratio, bool resolveInFullResolution);
	bool UnInit();

	bool& GetEnable() { return m_Enable; }
	bool& GetDebugDrawEnable() { return m_DebugDrawer.GetEnable(); }
	int32_t& GetAtrousLevel() { return m_AtrousLevel; }
	int32_t& GetRayReuseCount() { return m_RayReuseCount; }

	bool Reload();
	bool Resize(uint32_t width, uint32_t height);

	bool DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);
	bool Execute(KRHICommandList& commandList);

	IKRenderTargetPtr GetAOTarget() { return m_ComposeTarget; }
};