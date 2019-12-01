#pragma once

#include "Publish/KCamera.h"
#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/System/KCullSystem.h"

class KShadowMap
{
protected:
	std::vector<IKRenderTargetPtr> m_RenderTargets;

	IKSamplerPtr m_ShadowSampler;
	KCamera m_Camera;
	KCullSystem m_CullSystem;

	// Depth bias (and slope) are used to avoid shadowing artefacts
	// Constant depth bias factor (always applied)
	float m_DepthBiasConstant;
	// Slope depth bias factor, applied depending on polygon's slope
	float m_DepthBiasSlope;
public:
	KShadowMap();
	~KShadowMap();

	bool Init(IKRenderDevice* renderDevice,	size_t frameInFlight, size_t shadowMapSize);
	bool UnInit();

	bool UpdateShadowMap(IKRenderDevice* renderDevice, void* commandBufferPtr, size_t frameIndex);

	IKRenderTargetPtr GetShadowMapTarget(size_t frameIndex);
	inline IKSamplerPtr GetSampler() { return m_ShadowSampler; }
	KCamera& GetCamera() { return m_Camera; }

	inline float& GetDepthBiasConstant() { return m_DepthBiasConstant; }
	inline float& GetDepthBiasSlope() { return m_DepthBiasSlope; }
};