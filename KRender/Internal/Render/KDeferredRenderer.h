#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/Component/KRenderComponent.h"

enum DeferredRenderStage
{
	DRS_STATE_SKY,
	DRS_STAGE_PRE_PASS,
	DRS_STAGE_BASE_PASS,
	DRS_STAGE_DEFERRED_LIGHTING,
	DRS_STAGE_FORWARD_TRANSPRANT,
	DRS_STAGE_FORWARD_OPAQUE,
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
	{DRS_STATE_SKY, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "SkyPass"},
	{DRS_STAGE_PRE_PASS, PIPELINE_STAGE_PRE_Z, PIPELINE_STAGE_PRE_Z_INSTANCE, "PrePass"},
	{DRS_STAGE_BASE_PASS, PIPELINE_STAGE_BASEPASS, PIPELINE_STAGE_BASEPASS_INSTANCE, "BasePass"},
	{DRS_STAGE_DEFERRED_LIGHTING, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "LightingPass"},
	{DRS_STAGE_FORWARD_TRANSPRANT, PIPELINE_STAGE_TRANSPRANT, PIPELINE_STAGE_UNKNOWN, "TransprantPass"},
	{DRS_STAGE_FORWARD_OPAQUE, PIPELINE_STAGE_TRANSPRANT, PIPELINE_STAGE_UNKNOWN, "OpaquePass"},
	{DRS_STATE_DEBUG_OBJECT, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "DebugObjectPass"},
	{DRS_STATE_FOREGROUND, PIPELINE_STAGE_UNKNOWN, PIPELINE_STAGE_UNKNOWN, "ForegroundPass"}
};

class KCamera;

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

	IKRenderTargetPtr m_LightPassTarget;

	IKPipelinePtr m_LightingPipeline;
	IKPipelinePtr m_DrawFinalPipeline;

	KShaderRef m_QuadVS;
	KShaderRef m_DeferredLightingFS;
	KShaderRef m_SceneColorDrawFS;

	const KCamera* m_Camera;

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
	
	void SkyPass(IKCommandBufferPtr primaryBuffer);
	void PrePass(IKCommandBufferPtr primaryBuffer, const std::vector<KRenderComponent*>& cullRes);
	void BasePass(IKCommandBufferPtr primaryBuffer, const std::vector<KRenderComponent*>& cullRes);
	void DeferredLighting(IKCommandBufferPtr primaryBuffer);
	void ForwardTransprant(IKCommandBufferPtr primaryBuffer);
	void DebugObject(IKCommandBufferPtr primaryBuffer);
	void Foreground(IKCommandBufferPtr primaryBuffer);
	void EmptyAO(IKCommandBufferPtr primaryBuffer);

	void DrawFinalResult(IKRenderPassPtr renderPass, IKCommandBufferPtr buffer);

	inline IKRenderTargetPtr GetFinalSceneColor() { return m_LightPassTarget; }
};
