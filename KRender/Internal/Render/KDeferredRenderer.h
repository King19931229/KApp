#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/Render/KRenderUtil.h"

class KDeferredRenderer
{
public:
	typedef std::vector<RenderPassCallFuncType*> RenderPassCallFuncList;
protected:
	IKRenderPassPtr			m_RenderPass[DRS_STAGE_COUNT];
	KRenderStageStatistics	m_Statistics[DRS_STAGE_COUNT];
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

	void BuildMaterialSubMeshInstance(DeferredRenderStage renderStage, const std::vector<KRenderComponent*>& cullRes, std::vector<KMaterialSubMeshInstance>& instances);
	void HandleRenderCommandBinding(DeferredRenderStage renderStage, KRenderCommand& command);
	void BuildRenderCommand(KMultithreadingRenderContext& renderContext, DeferredRenderStage deferredRenderStage, const std::vector<KRenderComponent*>& cullRes);

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
	
	void PrePass(KMultithreadingRenderContext& renderContext, const std::vector<KRenderComponent*>& cullRes);
	void BasePass(KMultithreadingRenderContext& renderContext, const std::vector<KRenderComponent*>& cullRes);
	void DeferredLighting(IKCommandBufferPtr primaryBuffer);
	void ForwardTransprant(IKCommandBufferPtr primaryBuffer, const std::vector<KRenderComponent*>& cullRes);
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
