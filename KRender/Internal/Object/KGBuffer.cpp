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

	Resize(width, height);

	return true;
}

bool KGBuffer::UnInit()
{
	for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
	{
		SAFE_UNINIT(m_RenderTarget[i]);
	}
	SAFE_UNINIT(m_DepthStencilTarget);
	SAFE_UNINIT(m_AOTarget);
	SAFE_UNINIT(m_GBufferSampler);
	return true;
}

bool KGBuffer::Resize(uint32_t width, uint32_t height)
{
	if (width && height)
	{
		auto EnsureRenderTarget = [](IKRenderTargetPtr& target)
		{
			if (target) target->UnInit();
			else ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(target));
		};

		for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
		{
			EnsureRenderTarget(m_RenderTarget[i]);
			ASSERT_RESULT(m_RenderTarget[i]->InitFromColor(width, height, 1, 1, GBufferDescription[i].format));
		}

		EnsureRenderTarget(m_DepthStencilTarget);
		ASSERT_RESULT(m_DepthStencilTarget->InitFromDepthStencil(width, height, 1, true));

		EnsureRenderTarget(m_AOTarget);
		ASSERT_RESULT(m_AOTarget->InitFromColor(width, height, 1, 1, AOFormat));

		return true;
	}
	return false;
}

bool KGBuffer::TranslateColorAttachment(IKCommandBufferPtr buffer, ImageLayout layout)
{
	if (layout == IMAGE_LAYOUT_SHADER_READ_ONLY)
	{
		for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
		{
			buffer->Translate(m_RenderTarget[i]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
	}
	if (layout == IMAGE_LAYOUT_COLOR_ATTACHMENT)
	{
		for (uint32_t i = 0; i < GBUFFER_TARGET_COUNT; ++i)
		{
			buffer->Translate(m_RenderTarget[i]->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		}
	}
	return layout == IMAGE_LAYOUT_SHADER_READ_ONLY || layout == IMAGE_LAYOUT_COLOR_ATTACHMENT;
}

bool KGBuffer::TranslateDepthStencilAttachment(IKCommandBufferPtr buffer, ImageLayout layout)
{
	if (layout == IMAGE_LAYOUT_SHADER_READ_ONLY)
	{
		buffer->Translate(m_DepthStencilTarget->GetFrameBuffer(), IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}
	else
	{
		buffer->Translate(m_DepthStencilTarget->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
	}
	return layout == IMAGE_LAYOUT_SHADER_READ_ONLY || layout == IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
}