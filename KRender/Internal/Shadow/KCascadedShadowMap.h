#pragma once

#include "Publish/KCamera.h"
#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/System/KCullSystem.h"
#include "Internal/KStatistics.h"
#include "Internal/FrameGraph/KFrameGraph.h"

class KCascadedShadowMap;

class KCascadedShadowMapCasterPass : public KFrameGraphPass
{
protected:
	KCascadedShadowMap& m_Master;
	std::vector<KFrameGraphID> m_StaticTargetIDs;
	std::vector<KFrameGraphID> m_DynamicTargetIDs;
	std::vector<KFrameGraphID> m_AllTargetIDs;
	glm::mat4 m_LastCameraMatrix;
public:
	KCascadedShadowMapCasterPass(KCascadedShadowMap& master);
	~KCascadedShadowMapCasterPass();

	bool Init();
	bool UnInit();

	bool HasSideEffect() const override { return true; }

	bool Setup(KFrameGraphBuilder& builder) override;
	bool Execute(KFrameGraphExecutor& executor) override;

	IKRenderTargetPtr GetStaticTarget(size_t cascadedIndex);
	IKRenderTargetPtr GetDynamicTarget(size_t cascadedIndex);
	const std::vector<KFrameGraphID>& GetAllTargetID() const { return m_AllTargetIDs; }
};
typedef std::shared_ptr<KCascadedShadowMapCasterPass> KCascadedShadowMapCasterPassPtr;

class KCascadedShadowMapReceiverPass : public KFrameGraphPass
{
protected:
	KCascadedShadowMap& m_Master;
	KFrameGraphID m_StaticMaskID;
	KFrameGraphID m_DynamicMaskID;

	void Recreate();
public:
	KCascadedShadowMapReceiverPass(KCascadedShadowMap& master);
	~KCascadedShadowMapReceiverPass();

	bool Init();
	bool UnInit();

	bool HasSideEffect() const override { return true; }

	bool Setup(KFrameGraphBuilder& builder) override;
	bool Resize(KFrameGraphBuilder& builder) override;
	bool Execute(KFrameGraphExecutor& executor) override;

	KFrameGraphID GetStaticTargetID() const { return m_StaticMaskID; }
	KFrameGraphID GetDynamicTargetID() const { return m_DynamicMaskID; }
};
typedef std::shared_ptr<KCascadedShadowMapReceiverPass> KCascadedShadowMapReceiverPassPtr;

class KCascadedShadowMapDebugPass : public KFrameGraphPass
{
protected:
	KCascadedShadowMap& m_Master;
public:
	KCascadedShadowMapDebugPass(KCascadedShadowMap& master);
	~KCascadedShadowMapDebugPass();

	bool HasSideEffect() const override { return false; }

	bool Setup(KFrameGraphBuilder& builder) override;
	bool Execute(KFrameGraphExecutor& executor) override;
};
typedef std::shared_ptr<KCascadedShadowMapCasterPass> KCascadedShadowMapCasterPassPtr;

class KCascadedShadowMap
{
	friend class KCascadedShadowMapCasterPass;
	friend class KCascadedShadowMapReceiverPass;
	friend class KCascadedShadowMapDebugPass;
protected:
	enum
	{
		SHADOW_BINDING_GBUFFER_POSITION = SHADER_BINDING_TEXTURE0
	};

	static constexpr size_t SHADOW_MAP_MAX_CASCADED = 4;	
	const KCamera* m_MainCamera;

	// TODO 以下Debug数据需要共享
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_BackGroundVertices[4];
	static const uint16_t ms_BackGroundIndices[6];
	static const VertexFormat ms_VertexFormats[1];

	// Buffer
	IKVertexBufferPtr m_BackGroundVertexBuffer;
	IKIndexBufferPtr m_BackGroundIndexBuffer;

	// Shader
	IKShaderPtr m_DebugVertexShader;
	IKShaderPtr m_DebugFragmentShader;

	IKShaderPtr m_QuadVS;
	IKShaderPtr m_StaticReceiverFS;
	IKShaderPtr m_DynamicReceiverFS;
	IKShaderPtr m_CombineReceiverFS;

	IKPipelinePtr m_StaticReceiverPipeline;
	IKPipelinePtr m_DynamicReceiverPipeline;
	IKPipelinePtr m_CombineReceiverPipeline;

	IKRenderTargetPtr m_StaticMaskTarget;
	IKRenderTargetPtr m_DynamicMaskTarget;
	IKRenderPassPtr m_StaticReceiverPass;
	IKRenderPassPtr m_DynamicReceiverPass;

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

	KCascadedShadowMapCasterPassPtr m_CasterPass;
	KCascadedShadowMapReceiverPassPtr m_ReceiverPass;

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

	void UpdateCascades();
	bool GetDebugRenderCommand(KRenderCommandList& commands, bool isStatic);
	void PopulateRenderCommand(size_t cascadedIndex,
		IKRenderTargetPtr shadowTarget, IKRenderPassPtr renderPass,
		std::vector<KRenderComponent*>& litCullRes, std::vector<KRenderCommand>& commands, KRenderStageStatistics& statistics);
	void FilterRenderComponent(std::vector<KRenderComponent*>& in, bool isStatic);

	bool UpdateRT(size_t cascadedIndex, bool isStatic, IKCommandBufferPtr primaryBuffer, IKRenderTargetPtr shadowMapTarget, IKRenderPassPtr renderPass);
public:
	KCascadedShadowMap();
	~KCascadedShadowMap();

	bool Init(const KCamera* camera, size_t numCascaded, uint32_t shadowMapSize, float shadowSizeRatio, uint32_t width, uint32_t height);
	bool UnInit();

	bool UpdateShadowMap(IKCommandBufferPtr primaryBuffer);
	bool DebugRender(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers);
	bool Resize(uint32_t width, uint32_t height);

	IKRenderTargetPtr GetShadowMapTarget(size_t cascadedIndex, bool isStatic);

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
	inline KCascadedShadowMapCasterPassPtr GetCasterPass() { return m_CasterPass; }
	inline KCascadedShadowMapReceiverPassPtr GetReceiverPass() { return m_ReceiverPass; }
};