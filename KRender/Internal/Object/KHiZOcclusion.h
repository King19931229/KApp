#pragma once
#include "KHiZBuffer.h"
#include "KDebugDrawer.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include <list>

class KHiZOcclusion
{
public:
	enum
	{
		OCCLUSION_TARGRT_DIMENSION = 256,
		BLOCK_SIZE = 8,
		BLOCK_DIMENSON = OCCLUSION_TARGRT_DIMENSION / BLOCK_SIZE
	};
protected:
	struct Candidate
	{
		glm::vec3 pos;
		glm::vec3 extent;
		KRenderComponent* render = nullptr;
		std::weak_ptr<void> lifetime;
	};

	struct Batch
	{
		IKTexturePtr tempPosition;
		IKTexturePtr tempExtent;
		IKRenderTargetPtr position;
		IKRenderTargetPtr extent;
		IKRenderTargetPtr result;
		IKPipelinePtr preparePipeline;
		IKPipelinePtr executePipeline;
		IKRenderPassPtr preparePass;
		IKRenderPassPtr executePass;
		KRTDebugDrawer debugDrawer;
		IKFencePtr fence;
		std::vector<Candidate> candidates;
	};

	std::list<Batch> m_Batches;
	Batch* m_CurrentBatch = nullptr;
	IKRenderTargetPtr m_ReadbackTarget;

	KShaderRef m_QuadVS;
	KShaderRef m_PrepareCullFS;
	KShaderRef m_ExecuteCullFS;

	KSamplerRef m_Sampler;

	bool m_Enable;
	bool m_EnableDebugDraw;

	void InitBatch(Batch& batch);
	void ConfigurePipelines(Batch& batch);
	void PushCandidatesInformation(KRHICommandList& commandList, Batch& batch);
	void PullCandidatesResult();

	uint32_t CalcBlockX(uint32_t idx);
	uint32_t CalcBlockY(uint32_t idx);

	uint32_t CalcXInBlock(uint32_t idx);
	uint32_t CalcYInBlock(uint32_t idx);

	uint32_t CalcX(uint32_t idx);
	uint32_t CalcY(uint32_t idx);

	uint32_t XYToIndex(uint32_t x, uint32_t y);
public:
	KHiZOcclusion();
	~KHiZOcclusion();

	bool Init();
	bool UnInit();

	bool Reload();
	bool Resize();

	bool EnableDebugDraw();
	bool DisableDebugDraw();

	bool& GetEnable();
	bool& GetDebugDrawEnable();

	bool DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);
	bool Execute(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes);
};