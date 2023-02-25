#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/Component/KRenderComponent.h"

enum DeferredRenderStage
{
	DRS_STAGE_PRE_PASS,
	DRS_STAGE_BASE_PASS,

	DRS_STAGE_DEFERRED_LIGHTING,
	DRS_STAGE_FORWARD_TRANSPRANT,
	DRS_STAGE_FORWARD_OPAQUE,
	DRS_STATE_SKY,

	DRS_STATE_COPY_SCENE_COLOR,

	DRS_STATE_DEBUG_OBJECT,
	DRS_STATE_FOREGROUND,
	DRS_STAGE_COUNT
};

struct KDeferredRenderStageDescription
{
	DeferredRenderStage stage;
	PipelineStage pipelineStage;
	PipelineStage instancePipelineStage;
	const char* debugMakrer;
};

constexpr KDeferredRenderStageDescription GDeferredRenderStageDescription[DRS_STAGE_COUNT] =
{
	{DRS_STAGE_PRE_PASS, PIPELINE_STAGE_PRE_Z, PIPELINE_STAGE_PRE_Z_INSTANCE, "PrePass"},
	{DRS_STAGE_BASE_PASS, PIPELINE_STAGE_BASEPASS, PIPELINE_STAGE_BASEPASS_INSTANCE, "BasePass"},
	{DRS_STAGE_DEFERRED_LIGHTING, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "LightingPass"},
	{DRS_STAGE_FORWARD_TRANSPRANT, PIPELINE_STAGE_TRANSPRANT, PIPELINE_STAGE_UNKNOWN, "TransprantPass"},
	{DRS_STAGE_FORWARD_OPAQUE, PIPELINE_STAGE_TRANSPRANT, PIPELINE_STAGE_UNKNOWN, "OpaquePass"},
	{DRS_STATE_SKY, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "SkyPass"},
	{DRS_STATE_COPY_SCENE_COLOR, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "CopySceneColor"},
	{DRS_STATE_DEBUG_OBJECT, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "DebugObjectPass"},
	{DRS_STATE_FOREGROUND, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "ForegroundPass"}
};

enum DeferredRenderDebug
{
	DRD_NONE,

	DRD_ALBEDO,
	DRD_METAL,
	DRD_ROUGHNESS,
	DRD_AO,
	DRD_EMISSIVE,

	DRD_IBL_DIFFUSE,
	DRD_IBL_SPECULAR,
	DRD_IBL,

	DRD_DIRECTLIGHT_DIFFUSE,
	DRD_DIRECTLIGHT_SPECULAR,
	DRD_DIRECTLIGHT,

	DRD_NOV,
	DRD_NOL,
	DRD_VOH,

	DRD_NDF,
	DRD_G,
	DRD_F,

	DRD_KS,
	DRD_KD,

	DRD_DIRECT,
	DRD_INDIRECT,
	DRD_SCATTERING,

	DRD_COUNT
};

struct DeferredRenderDebugDescription
{
	DeferredRenderDebug debug;
	const char* name;
};

constexpr DeferredRenderDebugDescription GDeferredRenderDebugDescription[DRD_COUNT] =
{
	{ DRD_NONE, "None" },

	{ DRD_ALBEDO, "Albedo" },
	{ DRD_METAL, "Metal" },
	{ DRD_ROUGHNESS, "Roughness" },
	{ DRD_AO, "AO" },
	{ DRD_EMISSIVE, "Emissive" },

	{ DRD_IBL_DIFFUSE, "IBL Diffuse"},
	{ DRD_IBL_SPECULAR, "IBL Specular"},
	{ DRD_IBL, "IBL"},

	{ DRD_DIRECTLIGHT_DIFFUSE, "Direct Light Diffuse"},
	{ DRD_DIRECTLIGHT_SPECULAR, "Direct Light Specular"},
	{ DRD_DIRECTLIGHT, "Direct Light"},

	{ DRD_NOV, "N Dot V"},
	{ DRD_NOL, "N Dot L"},
	{ DRD_VOH, "V Dot H"},

	{ DRD_NDF, "Normal Distribution Function"},
	{ DRD_G, "Geometric Function" },
	{ DRD_F, "Fresnel Function" },

	{ DRD_KS, "IBL Specular Ratio"},
	{ DRD_KD, "IBL Diffuse Ratio"},

	{ DRD_DIRECT, "Direct"},
	{ DRD_INDIRECT, "Indirect"},
	{ DRD_SCATTERING, "Scattering"}
};

class KDeferredRenderer
{
public:
	typedef std::vector<RenderPassCallFuncType*> RenderPassCallFuncList;
protected:
	IKCommandPoolPtr		m_CommandPool;
	IKRenderPassPtr			m_RenderPass[DRS_STAGE_COUNT];
	KRenderStageStatistics	m_Statistics[DRS_STAGE_COUNT];
	IKCommandBufferPtr		m_CommandBuffers[DRS_STAGE_COUNT];
	RenderPassCallFuncList	m_RenderCallFuncs[DRS_STAGE_COUNT];
	IKRenderPassPtr			m_EmptyAORenderPass;

	IKRenderTargetPtr m_FinalTarget;

	IKPipelinePtr m_LightingPipeline;
	IKPipelinePtr m_DrawSceneColorPipeline;
	IKPipelinePtr m_DrawFinalPipeline;

	KShaderRef m_QuadVS;
	KShaderRef m_DeferredLightingFS;
	KShaderRef m_SceneColorDrawFS;

	DeferredRenderDebug m_DebugOption;

	const class KCamera* m_Camera;

	void BuildRenderCommand(IKCommandBufferPtr primaryBuffer, DeferredRenderStage renderStage, const std::vector<KRenderComponent*>& cullRes);

	void RecreateRenderPass(uint32_t width, uint32_t heigh);
	void RecreatePipeline();
public:
	KDeferredRenderer();
	~KDeferredRenderer();

	void Init(const KCamera* camera, uint32_t width, uint32_t height);
	void UnInit();
	void Resize(uint32_t width, uint32_t height);

	void AddCallFunc(DeferredRenderStage stage, RenderPassCallFuncType* func);
	void RemoveCallFunc(DeferredRenderStage stage, RenderPassCallFuncType* func);
	
	void PrePass(IKCommandBufferPtr primaryBuffer, const std::vector<KRenderComponent*>& cullRes);
	void BasePass(IKCommandBufferPtr primaryBuffer, const std::vector<KRenderComponent*>& cullRes);
	void DeferredLighting(IKCommandBufferPtr primaryBuffer);
	void ForwardTransprant(IKCommandBufferPtr primaryBuffer);
	void SkyPass(IKCommandBufferPtr primaryBuffer);
	void CopySceneColorToFinal(IKCommandBufferPtr primaryBuffer);
	void DebugObject(IKCommandBufferPtr primaryBuffer);
	void Foreground(IKCommandBufferPtr primaryBuffer);
	void EmptyAO(IKCommandBufferPtr primaryBuffer);

	void DrawFinalResult(IKRenderPassPtr renderPass, IKCommandBufferPtr buffer);

	inline IKRenderTargetPtr GetFinal() { return m_FinalTarget; }

	inline DeferredRenderDebug GetDebugOption() const { return m_DebugOption; }
	inline void SetDebugOption(DeferredRenderDebug option) { m_DebugOption = option; }

	void ReloadShader();
};
