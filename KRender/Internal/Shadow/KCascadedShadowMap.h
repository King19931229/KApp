#pragma once

#include "Publish/KCamera.h"
#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/System/KCullSystem.h"
#include "Internal/KStatistics.h"
#include "Internal/FrameGraph/KFrameGraph.h"

class KCascadedShadowMap;

class KCascadedShadowMapPass : public KFrameGraphPass
{
protected:
	KCascadedShadowMap& m_Master;
	std::vector<KFrameGraphID> m_TargetIDs;
public:
	KCascadedShadowMapPass(KCascadedShadowMap& master);
	~KCascadedShadowMapPass();

	bool Init();
	bool UnInit();

	bool HasSideEffect() const override { return true; }

	bool Setup(KFrameGraphBuilder& builder) override;
	bool Execute() override;

	IKRenderTargetPtr GetTarget(size_t cascadedIndex);
	const std::vector<KFrameGraphID>& GetAllTargetID() const { return m_TargetIDs; }
};
typedef std::shared_ptr<KCascadedShadowMapPass> KCascadedShadowMapPassPtr;

class KCascadedShadowMapDebugPass : public KFrameGraphPass
{
protected:
	KCascadedShadowMap& m_Master;
public:
	KCascadedShadowMapDebugPass(KCascadedShadowMap& master);
	~KCascadedShadowMapDebugPass();

	bool HasSideEffect() const override { return false; }

	bool Setup(KFrameGraphBuilder& builder) override;
	bool Execute() override;
};
typedef std::shared_ptr<KCascadedShadowMapPass> KCascadedShadowMapPassPtr;

class KCascadedShadowMap
{
	friend class KCascadedShadowMapPass;
	friend class KCascadedShadowMapDebugPass;
protected:
	static constexpr size_t SHADOW_MAP_MAX_CASCADED = 4;
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_BackGroundVertices[4];
	static const uint16_t ms_BackGroundIndices[6];
	static const VertexFormat ms_VertexFormats[1];
	
	IKRenderDevice* m_Device;

	// buffer
	IKVertexBufferPtr m_BackGroundVertexBuffer;
	IKIndexBufferPtr m_BackGroundIndexBuffer;

	// shader
	IKShaderPtr m_DebugVertexShader;
	IKShaderPtr m_DebugFragmentShader;

	KVertexData m_DebugVertexData;
	KIndexData m_DebugIndexData;

	struct Cascade
	{
		float splitDepth;
		uint32_t shadowSize;
		glm::mat4 viewMatrix;
		glm::mat4 viewProjMatrix;
		glm::vec4 viewInfo;
		// renderPass
		std::vector<IKRenderPassPtr> renderPasses;
		// debug
		glm::mat4 debugClip;
		IKPipelinePtr debugPipeline;
		// scene clipping
		KAABBBox frustumBox;
		KAABBBox litBox;
	};
	std::vector<Cascade> m_Cascadeds;

	KRenderStageStatistics m_Statistics;
	KCascadedShadowMapPassPtr m_Pass;

	IKSamplerPtr m_ShadowSampler;
	KCamera m_ShadowCamera;
	KCullSystem m_CullSystem;

	// Depth bias (and slope) are used to avoid shadowing artefacts
	// Constant depth bias factor (always applied)
	float m_DepthBiasConstant[SHADOW_MAP_MAX_CASCADED];
	// Slope depth bias factor, applied depending on polygon's slope
	float m_DepthBiasSlope[SHADOW_MAP_MAX_CASCADED];

	float m_LightSize;
	float m_ShadowRange;
	float m_SplitLambda;

	float m_ShadowSizeRatio;

	bool m_FixToScene;
	bool m_FixTexel;

	bool m_MinimizeShadowDraw;

	void UpdateCascades(const KCamera* mainCamera);
	bool GetDebugRenderCommand(KRenderCommandList& commands);
	void PopulateRenderCommand(size_t frameIndex, size_t cascadedIndex,
		IKRenderTargetPtr shadowTarget, IKRenderPassPtr renderPass,
		std::vector<KRenderComponent*>& litCullRes, std::vector<KRenderCommand>& commands, KRenderStageStatistics& statistics);

	bool UpdateRT(size_t frameIndex, size_t cascadedIndex, IKCommandBufferPtr primaryBuffer, IKRenderTargetPtr shadowMapTarget, IKRenderPassPtr renderPass);
public:
	KCascadedShadowMap();
	~KCascadedShadowMap();

	bool Init(IKRenderDevice* renderDevice, size_t frameInFlight, size_t numCascaded, uint32_t shadowMapSize, float shadowSizeRatio);
	bool UnInit();

	bool UpdateShadowMap(const KCamera* mainCamera, size_t frameIndex, IKCommandBufferPtr primaryBuffer);
	bool DebugRender(size_t frameIndex, IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers);

	IKRenderTargetPtr GetShadowMapTarget(size_t cascadedIndex);

	inline size_t GetNumCascaded() const { return m_Cascadeds.size(); }
	inline IKSamplerPtr GetSampler() { return m_ShadowSampler; }
	KCamera& GetCamera() { return m_ShadowCamera; }

	inline float& GetDepthBiasConstant(size_t index) { return m_DepthBiasConstant[index]; }
	inline float& GetDepthBiasSlope(size_t index) { return m_DepthBiasSlope[index]; }

	inline float& GetShadowRange() { return m_ShadowRange; }
	inline float& GetSplitLambda() { return m_SplitLambda; }
	inline float& GetLightSize() { return m_LightSize; }

	inline bool& GetFixToScene() { return m_FixToScene; }
	inline bool& GetFixTexel() { return m_FixTexel; }
	inline bool& GetMinimizeShadowDraw() { return m_MinimizeShadowDraw; }

	inline const KRenderStageStatistics& GetStatistics() const { return m_Statistics; }
};