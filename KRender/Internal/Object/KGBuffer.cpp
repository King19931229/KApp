#include "KGBuffer.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSampler.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKStatistics.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Dispatcher/KRenderUtil.h"

const KGBufferDescription GBufferDescription[GBUFFER_TARGET_COUNT] =
{
	{ GBUFFER_TARGET0, EF_R32G32B32A32_FLOAT, "xyz:world_normal w:depth" },
	{ GBUFFER_TARGET1, EF_R8GB8BA8_UNORM, "xy:motion zw:idle" },
	{ GBUFFER_TARGET2, EF_R8GB8BA8_UNORM, "xyz:diffuse_color w:idle" },
	{ GBUFFER_TARGET3, EF_R8GB8BA8_UNORM, "xyz:specular_color w:idle" },
};

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
			ASSERT_RESULT(m_RenderTarget[i]->InitFromColor(width, height, 1, GBufferDescription[i].format));
		}

		EnsureRenderTarget(m_DepthStencilTarget);
		ASSERT_RESULT(m_DepthStencilTarget->InitFromDepthStencil(width, height, 1, true));

		return true;
	}
	return false;
}