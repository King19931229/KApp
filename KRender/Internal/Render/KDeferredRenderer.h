#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/Render/KRenderUtil.h"
#include "Internal/Render/KRHICommandList.h"

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

	uint32_t m_CurrentVirtualFeedbackTargetBinding = 0;

	DeferredRenderDebug m_DebugOption;

	const class KCamera* m_Camera;

	void BuildMaterialSubMeshInstance(DeferredRenderStage renderStage, const std::vector<IKEntity*>& cullRes, std::vector<KMaterialSubMeshInstance>& instances);
	void HandleRenderCommandBinding(DeferredRenderStage renderStage, KRenderCommand& command);
	void BuildRenderCommand(KRHICommandList& commandList, DeferredRenderStage deferredRenderStage, const std::vector<IKEntity*>& cullRes);

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
	
	void PrePass(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes);
	void MainBasePass(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes);
	void PostBasePass(KRHICommandList& commandList);
	void DeferredLighting(KRHICommandList& commandList);
	void ForwardOpaque(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes);
	void ForwardTransprant(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes);
	void SkyPass(KRHICommandList& commandList);
	void CopySceneColorToFinal(KRHICommandList& commandList);
	void DebugObject(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes);
	void Foreground(KRHICommandList& commandList);
	void EmptyAO(KRHICommandList& commandList);

	void DrawFinalResult(IKRenderPassPtr renderPass, KRHICommandList& commandList);

	inline IKRenderTargetPtr GetFinal() { return m_FinalTarget; }

	inline DeferredRenderDebug GetDebugOption() const { return m_DebugOption; }
	inline void SetDebugOption(DeferredRenderDebug option) { m_DebugOption = option; }

	void Reload();
};
