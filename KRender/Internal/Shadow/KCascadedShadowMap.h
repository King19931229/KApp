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
	KFrameGraphID m_CombineMaskID;

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

	IKRenderTargetPtr GetStaticMask();
	IKRenderTargetPtr GetDynamicMask();
	IKRenderTargetPtr GetCombineMask();

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
public:
	static constexpr uint32_t SHADOW_MAP_MAX_CASCADED = 4;
protected:
	static constexpr ElementFormat RECEIVER_TARGET_FORMAT = EF_R8_UNORM;
	static constexpr char* RENDER_STAGE_NAME[2] = { "CSM_Static", "CSM_Dynamic" };
	const KCamera* m_MainCamera;

	// Shader
	KShaderRef m_DebugVertexShader;
	KShaderRef m_DebugFragmentShader;

	KShaderRef m_QuadVS;
	KShaderRef m_StaticReceiverFS;
	KShaderRef m_DynamicReceiverFS;
	KShaderRef m_CombineReceiverFS;

	IKPipelinePtr m_StaticReceiverPipeline;
	IKPipelinePtr m_DynamicReceiverPipeline;
	IKPipelinePtr m_CombineReceiverPipeline;

	IKRenderPassPtr m_StaticReceiverPass;
	IKRenderPassPtr m_DynamicReceiverPass;
	IKRenderPassPtr m_CombineReceiverPass;

	IKRenderTargetPtr m_StaticReceiverTarget;
	IKRenderTargetPtr m_DynamicReceiverTarget;
	IKRenderTargetPtr m_CombineReceiverTarget;

	struct Cascade
	{
		// resolution
		uint32_t shadowSize;
		// matrix
		glm::mat4 viewMatrix;
		glm::mat4 viewProjMatrix;
		glm::vec4 viewInfo;
		// scene clipping
		KAABBBox frustumBox;
		KAABBBox litBox;
		glm::vec4 clipPlanes[6];
		// renderPass
		IKRenderPassPtr renderPass;
		IKRenderTargetPtr rendertarget;
		// debug
		glm::mat4 debugClip;
		IKPipelinePtr debugPipeline;
		// parameters		
		float split;
		float areaSize;
	};
	std::vector<Cascade> m_StaticCascadeds;
	std::vector<Cascade> m_DynamicCascadeds;

	glm::vec3 m_StaticCenter;
	KRenderStageStatistics m_Statistics[2];

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

	bool m_Enable;

	bool m_FixToScene;
	bool m_FixTexel;
	bool m_MinimizeShadowDraw;

	bool m_StaticShouldUpdate;

	void UpdateDynamicCascades();
	void UpdateStaticCascades();
	void UpdateCascadesDebug();

	bool GetDebugRenderCommand(KRenderCommandList& commands, bool isStatic);
	void PopulateRenderCommand(size_t cascadedIndex, bool isStatic, const std::vector<KRenderComponent*>& litCullRes, std::vector<KRenderCommand>& commands, KRenderStageStatistics& statistics);
	void FilterRenderComponent(std::vector<KRenderComponent*>& in, bool isStatic);
	bool PopulateRenderCommandList(size_t cascadedIndex, bool isStatic, KRenderCommandList& commandList);

	bool UpdatePipelineFromRTChanged();

	bool UpdateRT(IKCommandBufferPtr primaryBuffer, IKRenderPassPtr renderPass, size_t cascadedIndex, bool isStatic);
	bool UpdateMask(IKCommandBufferPtr primaryBuffer, bool isStatic);
	bool CombineMask(IKCommandBufferPtr primaryBuffer);

	void ExecuteCasterUpdate(IKCommandBufferPtr commandBuffer, std::function<IKRenderTargetPtr(uint32_t, bool)> getCascadedTarget);

	enum MaskType
	{
		STATIC_MASK,
		DYNAMIC_MASK,
		COMBINE_MASK
	};
	void ExecuteMaskUpdate(IKCommandBufferPtr commandBuffer, std::function<IKRenderTargetPtr(MaskType)> getMaskTarget);

	inline IKRenderTargetPtr GetStaticTarget(uint32_t cascadedIndex) { return m_CasterPass ? m_CasterPass->GetStaticTarget(cascadedIndex) : m_StaticCascadeds[cascadedIndex].rendertarget; }
	inline IKRenderTargetPtr GetDynamicTarget(uint32_t cascadedIndex) { return m_CasterPass ? m_CasterPass->GetDynamicTarget(cascadedIndex) : m_DynamicCascadeds[cascadedIndex].rendertarget; }

	inline IKRenderTargetPtr GetStaticMask() { return m_ReceiverPass ? m_ReceiverPass->GetStaticMask() : m_StaticReceiverTarget; }
	inline IKRenderTargetPtr GetDynamicMask() { return m_ReceiverPass ? m_ReceiverPass->GetDynamicMask() : m_DynamicReceiverTarget; }
public:
	KCascadedShadowMap();
	~KCascadedShadowMap();

	bool Init(const KCamera* camera, uint32_t numCascaded, uint32_t shadowMapSize, uint32_t width, uint32_t height);
	bool UnInit();

	bool Resize();

	bool UpdateShadowMap();
	bool UpdateCasters(IKCommandBufferPtr commandBuffer);
	bool UpdateMask(IKCommandBufferPtr commandBuffer);

	bool DebugRender(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers);

	IKRenderTargetPtr GetShadowMapTarget(size_t cascadedIndex, bool isStatic);

	inline size_t GetNumCascaded() const { return m_DynamicCascadeds.size(); }
	inline IKSamplerPtr GetSampler() { return m_ShadowSampler; }
	KCamera& GetCamera() { return m_ShadowCamera; }

	inline float& GetDepthBiasConstant(size_t index) { return m_DepthBiasConstant[index]; }
	inline float& GetDepthBiasSlope(size_t index) { return m_DepthBiasSlope[index]; }

	inline float& GetShadowRange() { return m_ShadowRange; }
	inline float& GetSplitLambda() { return m_SplitLambda; }
	inline float& GetLightSize() { return m_LightSize; }

	inline bool& GetEnable() { return m_Enable; }
	inline bool& GetFixToScene() { return m_FixToScene; }
	inline bool& GetFixTexel() { return m_FixTexel; }
	inline bool& GetMinimizeShadowDraw() { return m_MinimizeShadowDraw; }

	inline KCascadedShadowMapCasterPassPtr GetCasterPass() { return m_CasterPass; }
	inline KCascadedShadowMapReceiverPassPtr GetReceiverPass() { return m_ReceiverPass; }

	inline IKRenderTargetPtr GetFinalMask() { return m_ReceiverPass ? m_ReceiverPass->GetCombineMask() : m_CombineReceiverTarget; }
};