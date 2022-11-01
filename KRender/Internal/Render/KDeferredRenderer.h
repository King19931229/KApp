#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/KVertexDefinition.h"

enum DeferredRenderStage
{
	DRS_STAGE_PRE_PASS,
	DRS_STAGE_BASE_PASS,
	DRS_STAGE_DEFERRED_LIGHTING,
	DRS_STAGE_FORWARD_TRANSPRANT,
	DRS_STAGE_COUNT
};

struct KDeferredRenderStageDescription
{
	DeferredRenderStage stage;
	PipelineStage pipelineStage;
	PipelineStage instancePipelineStage;
	const char* debugMakrer;
};

extern const KDeferredRenderStageDescription GDeferredRenderStageDescription[DRS_STAGE_COUNT];
class KCamera;

class KDeferredRenderer
{
protected:
	IKCommandPoolPtr m_CommandPool;
	IKRenderPassPtr m_RenderPass[DRS_STAGE_COUNT];
	KRenderStageStatistics m_Statistics[DRS_STAGE_COUNT];
	IKCommandBufferPtr m_CommandBuffers[DRS_STAGE_COUNT];
	IKRenderTargetPtr m_LightPassTarget;
	IKPipelinePtr m_LightingPipeline;
	IKShaderPtr m_LightingVS;
	IKShaderPtr m_LightingFS;
	const KCamera* m_Camera;

	void BuildRenderCommand(IKCommandBufferPtr primaryBuffer, DeferredRenderStage renderStage);

	void RecreateRenderPass(uint32_t width, uint32_t heigh);
	void RecreatePipeline();
public:
	KDeferredRenderer();
	~KDeferredRenderer();

	void Init(const KCamera* camera, uint32_t width, uint32_t height);
	void UnInit();
	void Resize(uint32_t width, uint32_t height);

	void PrePass(IKCommandBufferPtr primaryBuffer);
	void BasePass(IKCommandBufferPtr primaryBuffer);
	void DeferredLighting(IKCommandBufferPtr primaryBuffer);
	void ForwardTransprant(IKCommandBufferPtr primaryBuffer);
};
