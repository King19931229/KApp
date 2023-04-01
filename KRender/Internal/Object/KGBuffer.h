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
	GBUFFER_TARGET4,
	GBUFFER_TARGET_COUNT,
};

struct KGBufferDescription
{
	GBufferTarget target;
	ElementFormat format;
	const char* description;
};

constexpr KGBufferDescription GBufferDescription[GBUFFER_TARGET_COUNT]
{
	{ GBUFFER_TARGET0, EF_R16G16B16A16_FLOAT, "xyz:world_normal w:depth" },
	{ GBUFFER_TARGET1, EF_R16G16_FLOAT, "xy:motion" },
	{ GBUFFER_TARGET2, EF_R8G8B8A8_UNORM, "xyz:base_color w:ao" },
	{ GBUFFER_TARGET3, EF_R8G8_UNORM, "x:metal y:roughness zw:idle" },
	{ GBUFFER_TARGET4, EF_R8G8B8A8_UNORM, "xyz:emissive w:idle" },
};

constexpr ElementFormat AOFormat = EF_R8_UNORM;

class KGBuffer
{
protected:
	IKRenderTargetPtr m_RenderTarget[GBUFFER_TARGET_COUNT];
	IKRenderTargetPtr m_DepthStencilTarget;
	IKRenderTargetPtr m_SceneTarget;
	IKRenderTargetPtr m_AOTarget;
	IKSamplerPtr m_GBufferSampler;
	IKSamplerPtr m_GBufferClosestSampler;
public:
	KGBuffer();
	~KGBuffer();

	bool Init(uint32_t width, uint32_t height);
	bool UnInit();
	bool Resize(uint32_t width, uint32_t height);

	bool TranslateColorAttachment(IKCommandBufferPtr buffer, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	bool TranslateDepthStencilAttachment(IKCommandBufferPtr buffer, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);

	inline IKRenderTargetPtr GetGBufferTarget(GBufferTarget target) { return m_RenderTarget[target]; }
	inline IKRenderTargetPtr GetDepthStencilTarget() { return m_DepthStencilTarget; }
	inline IKRenderTargetPtr GetSceneColor() { return m_SceneTarget; }
	inline IKRenderTargetPtr GetAOTarget() { return m_AOTarget; }
	inline IKSamplerPtr GetSampler() { return m_GBufferSampler; }
	inline IKSamplerPtr GetClosestSampler() { return m_GBufferSampler; }
};