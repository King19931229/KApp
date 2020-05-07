#pragma once

#include "Publish/KCamera.h"
#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/System/KCullSystem.h"

class KCascadedShadowMap
{
protected:
	typedef std::vector<IKRenderTargetPtr> RenderTargetList;
	std::vector<RenderTargetList> m_CascadedTargets;

	IKSamplerPtr m_ShadowSampler;
	KCamera m_Camera;
	KCullSystem m_CullSystem;

	// Depth bias (and slope) are used to avoid shadowing artefacts
	// Constant depth bias factor (always applied)
	float m_DepthBiasConstant;
	// Slope depth bias factor, applied depending on polygon's slope
	float m_DepthBiasSlope;
public:
	KCascadedShadowMap();
	~KCascadedShadowMap();

	bool Init(IKRenderDevice* renderDevice, size_t frameInFlight, size_t numCascaded, size_t shadowMapSize);
	bool UnInit();

	bool UpdateShadowMap(IKRenderDevice* renderDevice, IKCommandBuffer* commandBuffer, size_t frameIndex);

	IKRenderTargetPtr GetShadowMapTarget(size_t cascadedIndex, size_t frameIndex);

	inline size_t GetNumCascaded() const { return m_CascadedTargets.size(); }
	inline IKSamplerPtr GetSampler() { return m_ShadowSampler; }
	KCamera& GetCamera() { return m_Camera; }

	inline float& GetDepthBiasConstant() { return m_DepthBiasConstant; }
	inline float& GetDepthBiasSlope() { return m_DepthBiasSlope; }
};