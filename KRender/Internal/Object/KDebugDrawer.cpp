#include "KDebugDrawer.h"
#include "Internal/KRenderGlobal.h"

KRTDebugDrawer::KRTDebugDrawer()
	: m_Pipeline(nullptr)
	, m_Enable(false)
{
}

KRTDebugDrawer::~KRTDebugDrawer()
{
	ASSERT_RESULT(!m_Pipeline);
	ASSERT_RESULT(!m_Target);
}

void KRTDebugDrawer::Move(KRTDebugDrawer&& rhs)
{
	m_Clip = rhs.m_Clip;
	m_Pipeline = std::move(rhs.m_Pipeline);
	m_Target = std::move(rhs.m_Target);
	m_Enable = rhs.m_Enable;
}

KRTDebugDrawer::KRTDebugDrawer(KRTDebugDrawer&& rhs)
{
	Move(std::move(rhs));
}

KRTDebugDrawer& KRTDebugDrawer::operator=(KRTDebugDrawer&& rhs)
{
	Move(std::move(rhs));
	return *this;
}

bool KRTDebugDrawer::Init(IKRenderTargetPtr target, float x, float y, float width, float height, bool linear)
{
	UnInit();

	m_Target = target;

	m_Rect.x = x;
	m_Rect.y = y;
	m_Rect.w = width;
	m_Rect.h = height;

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "others/debugquad.vert", m_VSShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "others/debugquadcolor.frag", m_FSShader, false));

	KSamplerDescription samplerDesc;

	samplerDesc.addressU = AM_REPEAT;
	samplerDesc.addressV = AM_REPEAT;
	samplerDesc.addressW = AM_REPEAT;

	samplerDesc.minFilter = linear ? FM_LINEAR : FM_NEAREST;
	samplerDesc.magFilter = linear ? FM_LINEAR : FM_NEAREST;

	ASSERT_RESULT(KRenderGlobal::SamplerManager.Acquire(samplerDesc, m_Sampler));

	KRenderGlobal::RenderDevice->CreatePipeline(m_Pipeline);

	m_Pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	m_Pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

	m_Pipeline->SetBlendEnable(true);
	m_Pipeline->SetCullMode(CM_BACK);
	m_Pipeline->SetFrontFace(FF_CLOCKWISE);

	m_Pipeline->SetDepthFunc(CF_ALWAYS, false, false);
	m_Pipeline->SetShader(ST_VERTEX, *m_VSShader);
	m_Pipeline->SetShader(ST_FRAGMENT, *m_FSShader);
	m_Pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_Target->GetFrameBuffer(), *m_Sampler, true);

	ASSERT_RESULT(m_Pipeline->Init());

	return true;
}

bool KRTDebugDrawer::UnInit()
{
	m_Target = nullptr;
	SAFE_UNINIT(m_Pipeline);
	m_VSShader.Release();
	m_FSShader.Release();
	m_Sampler.Release();
	return true;
}

void KRTDebugDrawer::EnableDraw()
{
	m_Enable = true;
}

void KRTDebugDrawer::DisableDraw()
{
	m_Enable = false;
}

bool KRTDebugDrawer::Render(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	if (m_Enable)
	{
		ASSERT_RESULT(m_Pipeline);

		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f)) * m_Clip;
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(m_Rect.w, m_Rect.h, 1.0f)) * m_Clip;
		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(m_Rect.x, m_Rect.y, 0.0f)) * m_Clip;
		m_Clip = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f)) * m_Clip;
		m_Clip = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)) * m_Clip;

		KRenderCommand command;

		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.pipeline = m_Pipeline;
		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = true;

		command.objectUsage.binding = SHADER_BINDING_OBJECT;
		command.objectUsage.range = sizeof(m_Clip);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&m_Clip, command.objectUsage);

		primaryBuffer->Render(command);
	}
	return true;
}