#pragma once
#include "KHiZBuffer.h"
#include "KDebugDrawer.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"

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
		KRenderComponent* render;
		Candidate()
		{
			render = nullptr;
		}
	};

	std::vector <IKTexturePtr> m_TempPositionTextures;
	std::vector <IKTexturePtr> m_TempExtentTextures;

	std::vector<IKRenderTargetPtr> m_PositionTargets;
	std::vector<IKRenderTargetPtr> m_ExtentTargets;
	std::vector<IKRenderTargetPtr> m_ResultTargets;
	std::vector<IKRenderTargetPtr> m_ResultReadbackTargets;

	KShaderRef m_QuadVS;
	KShaderRef m_PrepareCullFS;
	KShaderRef m_ExecuteCullFS;

	KSamplerRef m_Sampler;

	std::vector<IKPipelinePtr> m_PreparePipelines;
	std::vector<IKPipelinePtr> m_ExecutePipelines;

	std::vector<IKRenderPassPtr> m_PreparePasses;
	std::vector<IKRenderPassPtr> m_ExecutePasses;

	std::vector<KRTDebugDrawer> m_DebugDrawers;

	std::vector<std::vector<Candidate>> m_Candidates;

	uint32_t m_CandidateNum;
	uint32_t m_BlockX;
	uint32_t m_BlockY;

	bool m_Enable;
	bool m_EnableDebugDraw;

	void PushCandidatesInformation(KRHICommandList& commandList);
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

	bool ReloadShader();
	bool Resize();

	bool EnableDebugDraw();
	bool DisableDebugDraw();

	bool& GetEnable();
	bool& GetDebugDrawEnable();

	bool DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);
	bool Execute(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes);
};