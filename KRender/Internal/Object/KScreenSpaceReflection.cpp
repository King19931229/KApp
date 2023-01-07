#include "KScreenSpaceReflection.h"
#include "Internal/KRenderGlobal.h"

KScreenSpaceReflection::KScreenSpaceReflection()
	: m_Width(0)
	, m_Height(0)
	, m_Ratio(1.0f)
{
}

KScreenSpaceReflection::~KScreenSpaceReflection()
{
}

void KScreenSpaceReflection::InitializePipeline()
{
	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);

	m_ReflectionPipeline->UnInit();

	m_ReflectionPipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	m_ReflectionPipeline->SetShader(ST_VERTEX, *m_QuadVS);
	m_ReflectionPipeline->SetShader(ST_FRAGMENT, *m_ReflectionFS);

	m_ReflectionPipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	m_ReflectionPipeline->SetBlendEnable(false);
	m_ReflectionPipeline->SetCullMode(CM_NONE);
	m_ReflectionPipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	m_ReflectionPipeline->SetPolygonMode(PM_FILL);
	m_ReflectionPipeline->SetColorWrite(true, true, true, true);
	m_ReflectionPipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

	m_ReflectionPipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
	m_ReflectionPipeline->SetSampler(SHADER_BINDING_TEXTURE1, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET1)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
	m_ReflectionPipeline->SetSampler(SHADER_BINDING_TEXTURE2, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET2)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
	m_ReflectionPipeline->SetSampler(SHADER_BINDING_TEXTURE3, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET3)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
	m_ReflectionPipeline->SetSampler(SHADER_BINDING_TEXTURE4, KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
	m_ReflectionPipeline->SetSampler(SHADER_BINDING_TEXTURE5, KRenderGlobal::HiZBuffer.GetMaxBuffer()->GetFrameBuffer(), KRenderGlobal::HiZBuffer.GetHiZSampler(), false);

	m_ReflectionPipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

	m_ReflectionPipeline->Init();
}

bool KScreenSpaceReflection::Init(uint32_t width, uint32_t height, float ratio)
{
	UnInit();

	m_Ratio = ratio;

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_FinalTarget);
	KRenderGlobal::RenderDevice->CreatePipeline(m_ReflectionPipeline);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_ReflectionPass);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "ssr/ssr.frag", m_ReflectionFS, false);

	Resize(width, height);

	m_DebugDrawer.Init(m_FinalTarget, 0, 0, 1, 1);

	return true;
}

bool KScreenSpaceReflection::UnInit()
{
	m_DebugDrawer.UnInit();
	SAFE_UNINIT(m_FinalTarget);
	SAFE_UNINIT(m_ReflectionPipeline);
	SAFE_UNINIT(m_ReflectionPass);

	m_QuadVS.Release();
	m_ReflectionFS.Release();

	return true;
}

bool KScreenSpaceReflection::ReloadShader()
{
	if (m_QuadVS)
		m_QuadVS->Reload();
	if (m_ReflectionFS)
		m_ReflectionFS->Reload();
	if (m_ReflectionPipeline)
		m_ReflectionPipeline->Reload();

	return true;
}

bool KScreenSpaceReflection::Resize(uint32_t width, uint32_t height)
{
	m_Width = std::max((uint32_t)(width * m_Ratio), 1U);
	m_Height = std::max((uint32_t)(height * m_Ratio), 1U);
	m_FinalTarget->UnInit();
	m_FinalTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R8GB8BA8_UNORM);

	m_ReflectionPass->UnInit();
	m_ReflectionPass->SetColorAttachment(0, m_FinalTarget->GetFrameBuffer());
	m_ReflectionPass->SetOpColor(0, LO_CLEAR, SO_STORE);
	m_ReflectionPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	ASSERT_RESULT(m_ReflectionPass->Init());

	InitializePipeline();

	return true;
}

bool KScreenSpaceReflection::DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	return m_DebugDrawer.Render(renderPass, primaryBuffer);
}

bool KScreenSpaceReflection::Execute(IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker("SSR", glm::vec4(0, 1, 0, 0));
	{
		primaryBuffer->Translate(KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->BeginRenderPass(m_ReflectionPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_ReflectionPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		command.pipeline = m_ReflectionPipeline;
		command.pipeline->GetHandle(m_ReflectionPass, command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Translate(m_FinalTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->Translate(KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	}
	primaryBuffer->EndDebugMarker();

	return true;
}