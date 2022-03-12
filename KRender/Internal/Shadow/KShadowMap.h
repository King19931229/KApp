#pragma once

#include "Publish/KCamera.h"
#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/System/KCullSystem.h"

class KShadowMap
{
protected:
	IKRenderTargetPtr m_RenderTarget;
	IKRenderPassPtr m_RenderPass;
	IKSamplerPtr m_ShadowSampler;
	IKCommandBufferPtr m_CommandBuffer;
	IKCommandPoolPtr m_CommandPool;

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

	bool Init(IKRenderDevice* renderDevice,	uint32_t shadowMapSize);
	bool UnInit();

	bool UpdateShadowMap(IKCommandBufferPtr primaryBuffer);

	inline IKRenderTargetPtr GetShadowMapTarget() { return m_RenderTarget; }
	inline IKSamplerPtr GetSampler() { return m_ShadowSampler; }
	inline KCamera& GetCamera() { return m_Camera; }

	inline float& GetDepthBiasConstant() { return m_DepthBiasConstant; }
	inline float& GetDepthBiasSlope() { return m_DepthBiasSlope; }
};