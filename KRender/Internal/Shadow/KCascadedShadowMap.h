#pragma once

#include "Publish/KCamera.h"
#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/System/KCullSystem.h"

class KCascadedShadowMap
{
protected:
	static constexpr size_t SHADOW_MAP_MAX_CASCADED = 4;
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_BackGroundVertices[4];
	static const uint16_t ms_BackGroundIndices[6];
	static const VertexFormat ms_VertexFormats[1];
	
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
		size_t shadowSize;
		glm::mat4 viewProjMatrix;
		glm::vec2 viewNearFar;

		std::vector<IKRenderTargetPtr> renderTargets;
		std::vector<IKCommandBufferPtr> commandBuffers;
		// debug
		glm::mat4 debugClip;
		std::vector<IKPipelinePtr> debugPipelines;
		// scene clipping
		KAABBBox frustumBox;
		KAABBBox litBox;
	};
	std::vector<Cascade> m_Cascadeds;

	std::vector<IKCommandBufferPtr> m_DebugCommandBuffers;
	IKCommandPoolPtr m_CommandPool;

	IKSamplerPtr m_ShadowSampler;
	KCamera m_ShadowCamera;
	KCullSystem m_CullSystem;

	// Depth bias (and slope) are used to avoid shadowing artefacts
	// Constant depth bias factor (always applied)
	float m_DepthBiasConstant;
	// Slope depth bias factor, applied depending on polygon's slope
	float m_DepthBiasSlope;

	float m_ShadowRange;
	float m_SplitLambda;
	
	float m_ShadowSizeRatio;

	bool m_FixToScene;
	bool m_FixTexel;

	bool m_MinimizeShadowDraw;

	void UpdateCascades(const KCamera* mainCamera);
	bool GetDebugRenderCommand(size_t frameIndex, KRenderCommandList& commands);
public:
	KCascadedShadowMap();
	~KCascadedShadowMap();

	bool Init(IKRenderDevice* renderDevice, size_t frameInFlight, size_t numCascaded, size_t shadowMapSize, float shadowSizeRatio);
	bool UnInit();

	bool UpdateShadowMap(const KCamera* mainCamera, size_t frameIndex, IKCommandBufferPtr primaryBuffer);
	bool DebugRender(size_t frameIndex, IKRenderTargetPtr target, std::vector<IKCommandBufferPtr>& buffers);

	IKRenderTargetPtr GetShadowMapTarget(size_t cascadedIndex, size_t frameIndex);

	inline size_t GetNumCascaded() const { return m_Cascadeds.size(); }
	inline IKSamplerPtr GetSampler() { return m_ShadowSampler; }
	KCamera& GetCamera() { return m_ShadowCamera; }

	inline float& GetDepthBiasConstant() { return m_DepthBiasConstant; }
	inline float& GetDepthBiasSlope() { return m_DepthBiasSlope; }

	inline float& GetShadowRange() { return m_ShadowRange; }
	inline float& GetSplitLambda() { return m_SplitLambda; }

	inline bool& GetFixToScene() { return m_FixToScene; }
	inline bool& GetFixTexel() { return m_FixTexel; }
	inline bool& GetMinimizeShadowDraw() { return m_MinimizeShadowDraw; }
};