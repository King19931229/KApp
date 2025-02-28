#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/Render/KRHICommandList.h"
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

constexpr ElementFormat AOFormat = EF_R8_UNORM;//EF_R8G8B8A8_UNORM;
constexpr ElementFormat VirtualTextureFeedbackFormat = EF_R8G8B8A8_UNORM;//EF_R8G8B8A8_UNORM;

class KGBuffer
{
protected:
	IKRenderTargetPtr m_RenderTarget[GBUFFER_TARGET_COUNT];
	IKRenderTargetPtr m_DepthStencilTarget;
	IKRenderTargetPtr m_OpaqueColorCopyTarget;
	IKRenderTargetPtr m_SceneColorTarget;
	IKRenderTargetPtr m_AOTarget;
	IKSamplerPtr m_GBufferSampler;
	IKSamplerPtr m_GBufferClosestSampler;

	uint32_t m_Width;
	uint32_t m_Height;
public:
	KGBuffer();
	~KGBuffer();

	bool Init(uint32_t width, uint32_t height);
	bool UnInit();
	bool Resize(uint32_t width, uint32_t height);

	bool TransitionOpaqueColorCopy(KRHICommandList& commandList, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	bool TransitionGBuffer(KRHICommandList& commandList, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	bool TransitionDepthStencil(KRHICommandList& commandList, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	bool TransitionAO(KRHICommandList& commandList, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);

	inline IKRenderTargetPtr GetGBufferTarget(GBufferTarget target) { return m_RenderTarget[target]; }
	inline IKRenderTargetPtr GetDepthStencilTarget() { return m_DepthStencilTarget; }
	inline IKRenderTargetPtr GetOpaqueColorCopy() { return m_OpaqueColorCopyTarget; }
	inline IKRenderTargetPtr GetSceneColor() { return m_SceneColorTarget; }
	inline IKRenderTargetPtr GetAOTarget() { return m_AOTarget; }
	inline IKSamplerPtr GetSampler() { return m_GBufferSampler; }
	inline IKSamplerPtr GetClosestSampler() { return m_GBufferSampler; }

	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }
};