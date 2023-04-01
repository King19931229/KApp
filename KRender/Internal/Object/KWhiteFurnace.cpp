#include "KWhiteFurnace.h"
#include "Internal/KRenderGlobal.h"

KWhiteFurnace::KWhiteFurnace()
	: m_WFTestPipeline(nullptr)
	, m_WFTarget(nullptr)
	, m_CommandBuffer(nullptr)
	, m_CommandPool(nullptr)
{
}

KWhiteFurnace::~KWhiteFurnace()
{
	ASSERT_RESULT(!m_WFTestPipeline);
	ASSERT_RESULT(!m_WFTarget);
	ASSERT_RESULT(!m_CommandBuffer);
	ASSERT_RESULT(!m_CommandPool);
}

bool KWhiteFurnace::Init()
{
	KRenderGlobal::RenderDevice->CreateComputePipeline(m_WFTestPipeline);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_WFTarget);

	m_WFTarget->InitFromStorage(1024, 1024, 1, EF_R32G32B32A32_FLOAT);
	m_WFTestPipeline->BindStorageImage(0, m_WFTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
	m_WFTestPipeline->Init("pbr/white _furnace.comp");

	KRenderGlobal::RenderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_GRAPHICS, 0);
	KRenderGlobal::RenderDevice->CreateCommandBuffer(m_CommandBuffer);
	m_CommandBuffer->Init(m_CommandPool, CBL_PRIMARY);

	Execute();

	SAFE_UNINIT(m_WFTestPipeline);
	SAFE_UNINIT(m_CommandBuffer);
	SAFE_UNINIT(m_CommandPool);

	return true;
}

bool KWhiteFurnace::UnInit()
{
	SAFE_UNINIT(m_WFTarget);
	return true;
}

bool KWhiteFurnace::Execute()
{
	m_CommandBuffer->BeginPrimary();

	const uint32_t GROUP_SIZE = 32;

	m_CommandBuffer->Translate(m_WFTarget->GetFrameBuffer(), PIPELINE_STAGE_BOTTOM_OF_PIPE, PIPELINE_STAGE_TOP_OF_PIPE, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_GENERAL);

	m_WFTestPipeline->Execute(m_CommandBuffer,
		(uint32_t)(m_WFTarget->GetFrameBuffer()->GetWidth() + GROUP_SIZE - 1) / GROUP_SIZE,
		(uint32_t)(m_WFTarget->GetFrameBuffer()->GetHeight() + GROUP_SIZE - 1) / GROUP_SIZE,
		6);

	m_CommandBuffer->Translate(m_WFTarget->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);

	m_CommandBuffer->End();
	m_CommandBuffer->Flush();

	return true;
}