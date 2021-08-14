#include "KRTAO.h"
#include "Internal/KRenderGlobal.h"
#include "interface/IKComputePipeline.h"

KRTAO::KRTAO()
	: m_Camera(nullptr)
	, m_PrevCamMat(glm::mat4(1.0f))
	, m_Width(1024)
	, m_Height(1024)
{
}

KRTAO::~KRTAO()
{
	ASSERT_RESULT(!m_ComputePipeline);
	ASSERT_RESULT(!m_RenderTarget);
	ASSERT_RESULT(!m_UniformBuffer);
}

void KRTAO::UpdateUniform()
{
	if (m_UniformBuffer)
	{
		glm::mat4 camMat = m_Camera->GetViewMatrix();
		if (memcmp(&m_PrevCamMat, &camMat, sizeof(camMat)) != 0)
		{
			m_PrevCamMat = camMat;
			m_AOParameters.frame = 0;
		}
		else
		{
			++m_AOParameters.frame;
		}

		m_UniformBuffer->Write(&m_AOParameters);
	}
}

bool KRTAO::Init(IKRayTraceScene* scene)
{
	if (scene)
	{
		IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

		renderDevice->CreateRenderTarget(m_RenderTarget);
		UpdateSize();

		m_Camera = scene->GetCamera();
		m_PrevCamMat = glm::mat4(0.0f);

		renderDevice->CreateUniformBuffer(m_UniformBuffer);
		m_UniformBuffer->InitMemory(sizeof(m_AOParameters), &m_AOParameters);
		m_UniformBuffer->InitDevice();

		IKRayTracePipeline* rayPipeline = scene->GetRayTracePipeline();

		IKRenderTargetPtr normalBuffer = KRenderGlobal::GBuffer.GetGBufferTarget0();
		IKRenderTargetPtr positionBuffer = KRenderGlobal::GBuffer.GetGBufferTarget1();

		renderDevice->CreateComputePipeline(m_ComputePipeline);
		m_ComputePipeline->BindStorageImage(BINDING_GBUFFER_NORMAL, normalBuffer, true, true);
		m_ComputePipeline->BindStorageImage(BINDING_GBUFFER_POSITION, positionBuffer, true, true);
		m_ComputePipeline->BindAccelerationStructure(BINDING_AS, rayPipeline->GetTopdownAS(), true);
		m_ComputePipeline->BindUniformBuffer(BDINING_UNIFORM, m_UniformBuffer, false);
		m_ComputePipeline->BindStorageImage(BINDING_OUT, m_RenderTarget, false, true);
		m_ComputePipeline->Init("ao/rtao.comp");
	}
	m_DebugDrawer.Init(m_RenderTarget);
	return true;
}

bool KRTAO::UnInit()
{
	m_DebugDrawer.UnInit();
	SAFE_UNINIT(m_ComputePipeline);
	SAFE_UNINIT(m_RenderTarget);
	SAFE_UNINIT(m_UniformBuffer);
	m_Camera = nullptr;
	return true;
}

bool KRTAO::EnableDebugDraw(float x, float y, float width, float height)
{
	m_DebugDrawer.EnableDraw(x, y, width, height);
	return true;
}

bool KRTAO::DisableDebugDraw()
{
	m_DebugDrawer.DisableDraw();
	return true;
}

bool KRTAO::GetDebugRenderCommand(KRenderCommandList& commands)
{
	return m_DebugDrawer.GetDebugRenderCommand(commands);
}

bool KRTAO::Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (m_ComputePipeline)
	{
		UpdateUniform();
		const uint32_t GROUP_SIZE = 16;
		uint32_t groupX = (m_Width + (GROUP_SIZE - 1)) / GROUP_SIZE;
		uint32_t groupY = (m_Height + (GROUP_SIZE - 1)) / GROUP_SIZE;
		m_ComputePipeline->Execute(primaryBuffer, groupX, groupY, 1, frameIndex);
	}
	return true;
}

void KRTAO::UpdateSize()
{
	if (m_RenderTarget)
	{
		IKSwapChain* chain = KRenderGlobal::RenderDevice->GetSwapChain();		
		if (chain->GetWidth() && chain->GetHeight())
		{
			m_Width = chain->GetWidth();
			m_Height = chain->GetHeight();
		}
		m_RenderTarget->UnInit();
		m_RenderTarget->InitFromStorage(m_Width, m_Height, EF_R32_FLOAT);
	}
}