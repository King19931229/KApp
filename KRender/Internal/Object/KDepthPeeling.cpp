#include "KDepthPeeling.h"
#include "Internal/KRenderGlobal.h"

KDepthPeeling::KDepthPeeling()
	: m_Width(0)
	, m_Height(0)
	, m_PeelingLayers(0)
{}

KDepthPeeling::~KDepthPeeling()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		ASSERT_RESULT(m_PeelingDepthTarget[i] == nullptr);
		ASSERT_RESULT(m_PeelingPass[i] == nullptr);
	}
}

bool KDepthPeeling::Init(uint32_t width, uint32_t height, uint32_t layers)
{
	UnInit();
	m_PeelingLayers = layers;
	Resize(width, height);
	return true;
}

bool KDepthPeeling::UnInit()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		SAFE_UNINIT(m_PeelingDepthTarget[i]);
		SAFE_UNINIT(m_PeelingPass[i]);
	}
	return true;
}

bool KDepthPeeling::Resize(uint32_t width, uint32_t height)
{
	if (m_Width == width && m_Height == height)
	{
		return true;
	}

	m_Width = width;
	m_Height = height;

	for (uint32_t i = 0; i < 2; ++i)
	{
		if (!m_PeelingDepthTarget[i])
		{
			KRenderGlobal::RenderDevice->CreateRenderTarget(m_PeelingDepthTarget[i]);
		}
		else
		{
			m_PeelingDepthTarget[i]->UnInit();
		}
		m_PeelingDepthTarget[i]->InitFromDepthStencil(m_Width, m_Height, 1, false);
	}

	for (uint32_t i = 0; i < 2; ++i)
	{
		if (!m_PeelingPass[i])
		{
			KRenderGlobal::RenderDevice->CreateRenderPass(m_PeelingPass[i]);
		}
		else
		{
			m_PeelingPass[i]->UnInit();
		}
		m_PeelingPass[i]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		m_PeelingPass[i]->SetOpColor(0, LO_CLEAR, SO_STORE);
		m_PeelingPass[i]->SetClearDepthStencil({ 1.0f, 0 });
		m_PeelingPass[i]->SetOpDepthStencil(LO_CLEAR, SO_STORE, LO_CLEAR, SO_STORE);
		m_PeelingPass[i]->Init();
	}

	return true;
}

bool KDepthPeeling::Execute(KRHICommandList& commandList)
{
	for (uint32_t i = 0; i < m_PeelingLayers; ++i)
	{

	}

	return true;
}