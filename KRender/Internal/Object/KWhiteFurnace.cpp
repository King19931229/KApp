#include "KWhiteFurnace.h"
#include "Internal/KRenderGlobal.h"

KWhiteFurnace::KWhiteFurnace()
	: m_WFTestPipeline(nullptr)
	, m_WFTarget(nullptr)
{
}

KWhiteFurnace::~KWhiteFurnace()
{
	ASSERT_RESULT(!m_WFTestPipeline);
	ASSERT_RESULT(!m_WFTarget);
}

bool KWhiteFurnace::Init()
{
	KRenderGlobal::RenderDevice->CreateComputePipeline(m_WFTestPipeline);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_WFTarget);

	m_WFTarget->InitFromStorage(1024, 1024, 1, EF_R32G32B32A32_FLOAT);
	m_WFTestPipeline->BindStorageImage(0, m_WFTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
	m_WFTestPipeline->Init("pbr/white _furnace.comp");

	Execute();

	SAFE_UNINIT(m_WFTestPipeline);

	return true;
}

bool KWhiteFurnace::UnInit()
{
	SAFE_UNINIT(m_WFTarget);
	return true;
}

bool KWhiteFurnace::Execute()
{
	IKCommandBufferPtr commandBuffer = KRenderGlobal::CommandPool->Request(CBL_PRIMARY);

	commandBuffer->BeginPrimary();

	const uint32_t GROUP_SIZE = 32;

	// m_CommandBuffer->Transition(m_WFTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);

	m_WFTestPipeline->Execute(commandBuffer,
		(uint32_t)(m_WFTarget->GetFrameBuffer()->GetWidth() + GROUP_SIZE - 1) / GROUP_SIZE,
		(uint32_t)(m_WFTarget->GetFrameBuffer()->GetHeight() + GROUP_SIZE - 1) / GROUP_SIZE,
		6);

	// m_CommandBuffer->Transition(m_WFTarget->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);

	commandBuffer->End();
	commandBuffer->Flush();

	return true;
}