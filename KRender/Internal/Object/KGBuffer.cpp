#include "KGBuffer.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSampler.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKStatistics.h"
#include "Internal/KRenderGlobal.h"

static_assert(GBufferDescription[GBUFFER_TARGET0].target == GBUFFER_TARGET0, "check");
static_assert(GBufferDescription[GBUFFER_TARGET1].target == GBUFFER_TARGET1, "check");
static_assert(GBufferDescription[GBUFFER_TARGET2].target == GBUFFER_TARGET2, "check");
static_assert(GBufferDescription[GBUFFER_TARGET3].target == GBUFFER_TARGET3, "check");

KGBuffer::KGBuffer()
	: m_Width(0)
	, m_Height(0)
{}

KGBuffer::~KGBuffer()
{
	ASSERT_RESULT(m_DepthStencilTarget == nullptr);
	ASSERT_RESULT(m_GBufferSampler == nullptr);
	for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
	{
		ASSERT_RESULT(m_RenderTarget[i] == nullptr);
	}
}

bool KGBuffer::Init(uint32_t width, uint32_t height)
{
	ASSERT_RESULT(UnInit());

	KRenderGlobal::RenderDevice->CreateSampler(m_GBufferSampler);
	m_GBufferSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_GBufferSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_GBufferSampler->Init(0, 0);

	KRenderGlobal::RenderDevice->CreateSampler(m_GBufferClosestSampler);
	m_GBufferClosestSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_GBufferClosestSampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	m_GBufferClosestSampler->Init(0, 0);

	Resize(width, height);

	return true;
}

bool KGBuffer::UnInit()
{
	for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
	{
		SAFE_UNINIT(m_RenderTarget[i]);
	}
	SAFE_UNINIT(m_OpaqueColorCopyTarget);
	SAFE_UNINIT(m_SceneColorTarget);
	SAFE_UNINIT(m_DepthStencilTarget);
	SAFE_UNINIT(m_AOTarget);
	SAFE_UNINIT(m_GBufferSampler);
	SAFE_UNINIT(m_GBufferClosestSampler);

	m_Width = 0;
	m_Height = 0;
	return true;
}

bool KGBuffer::Resize(uint32_t width, uint32_t height)
{
	if (width && height)
	{
		if (width == m_Width && height == m_Height)
		{
			return true;
		}

		auto EnsureRenderTarget = [](IKRenderTargetPtr& target)
		{
			if (target) target->UnInit();
			else ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(target));
		};

		for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
		{
			EnsureRenderTarget(m_RenderTarget[i]);
			ASSERT_RESULT(m_RenderTarget[i]->InitFromColor(width, height, 1, 1, GBufferDescription[i].format));
			m_RenderTarget[i]->GetFrameBuffer()->SetDebugName(("GBuffer_" + std::to_string(i)).c_str());
		}

		EnsureRenderTarget(m_DepthStencilTarget);
		ASSERT_RESULT(m_DepthStencilTarget->InitFromDepthStencil(width, height, 1, true));
		m_DepthStencilTarget->GetFrameBuffer()->SetDebugName("DepthStencil");

		EnsureRenderTarget(m_AOTarget);
		ASSERT_RESULT(m_AOTarget->InitFromColor(width, height, 1, 1, AOFormat));
		m_AOTarget->GetFrameBuffer()->SetDebugName("AO");

		EnsureRenderTarget(m_OpaqueColorCopyTarget);
		ASSERT_RESULT(m_OpaqueColorCopyTarget->InitFromColor(width, height, 1, 1, EF_R16G16B16A16_FLOAT));
		m_OpaqueColorCopyTarget->GetFrameBuffer()->SetDebugName("OpaqueColorCopy");

		EnsureRenderTarget(m_SceneColorTarget);
		ASSERT_RESULT(m_SceneColorTarget->InitFromColor(width, height, 1, 1, EF_R16G16B16A16_FLOAT));
		m_SceneColorTarget->GetFrameBuffer()->SetDebugName("SceneColor");

		m_Width = width;
		m_Height = height;

		return true;
	}
	return false;
}

bool KGBuffer::TransitionOpaqueColorCopy(KRHICommandList& commandList, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	commandList.TransitionOwnership(m_OpaqueColorCopyTarget->GetFrameBuffer(), srcQueue, dstQueue, srcStages, dstStages, oldLayout, newLayout);
	return true;
}

bool KGBuffer::TransitionGBuffer(KRHICommandList& commandList, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
	{
		commandList.TransitionOwnership(m_RenderTarget[i]->GetFrameBuffer(), srcQueue, dstQueue, srcStages, dstStages, oldLayout, newLayout);
	}
	return true;
}

bool KGBuffer::TransitionDepthStencil(KRHICommandList& commandList, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	commandList.TransitionOwnership(m_DepthStencilTarget->GetFrameBuffer(), srcQueue, dstQueue, srcStages, dstStages, oldLayout, newLayout);
	return true;
}

bool KGBuffer::TransitionAO(KRHICommandList& commandList, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	commandList.TransitionOwnership(m_AOTarget->GetFrameBuffer(), srcQueue, dstQueue, srcStages, dstStages, oldLayout, newLayout);
	return true;
}