#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Publish/KCamera.h"

enum GBufferTarget
{
	GBUFFER_TARGET0,
	GBUFFER_TARGET1,	
	GBUFFER_TARGET2,
	GBUFFER_TARGET3,
	GBUFFER_TARGET_COUNT,
};

struct KGBufferDescription
{
	GBufferTarget target;
	ElementFormat format;
	const char* description;
};

extern const KGBufferDescription GBufferDescription[GBUFFER_TARGET_COUNT];

class KGBuffer
{
protected:
	IKRenderTargetPtr m_RenderTarget[GBUFFER_TARGET_COUNT];
	IKRenderTargetPtr m_DepthStencilTarget;
	IKSamplerPtr m_GBufferSampler;
public:
	KGBuffer();
	~KGBuffer();

	bool Init(uint32_t width, uint32_t height);
	bool UnInit();
	bool Resize(uint32_t width, uint32_t height);

	inline IKRenderTargetPtr GetGBufferTarget(GBufferTarget target) { return m_RenderTarget[target]; }
	inline IKRenderTargetPtr GetDepthStencilTarget() { return m_DepthStencilTarget; }
	inline IKSamplerPtr GetSampler() { return m_GBufferSampler; }
};