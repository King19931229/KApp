#include "KScreenSpaceReflection.h"
#include "Internal/KRenderGlobal.h"

KScreenSpaceReflection::KScreenSpaceReflection()
	: m_Width(0)
	, m_Height(0)
	, m_Ratio(1.0f)
	, m_AtrousLevel(0)
	, m_CurrentIdx(0)
	, m_FirstFrame(false)
{
}

KScreenSpaceReflection::~KScreenSpaceReflection()
{
}

void KScreenSpaceReflection::InitializePipeline()
{
	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);

	{
		IKPipelinePtr& pipeline = m_ReflectionPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_ReflectionFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET1)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET2)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE3, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET3)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE4, KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE5, KRenderGlobal::HiZBuffer.GetMinBuffer()->GetFrameBuffer(), KRenderGlobal::HiZBuffer.GetHiZSampler(), false);

		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_RayReusePipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_RayReuseFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_HitResultTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_HitMaskTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE3, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);

		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	for (uint32_t i = 0; i < 2; ++i)
	{
		IKPipelinePtr& pipeline = m_TemporalPipeline[i];
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_TemporalFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_TemporalTarget[(i + 1) & 1]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_TemporalTarget[i]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_HitResultTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE3, m_TemporalSquaredTarget[(i + 1) & 1]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE4, m_TemporalTsppTarget[(i + 1) & 1]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE5, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);

		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_BlitPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_BlitFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_FinalTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_FinalSquaredTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_FinalTsppTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);

		pipeline->Init();
	}

	for (uint32_t i = 0; i < 2; ++i)
	{
		IKPipelinePtr& pipeline = m_AtrousPipeline[i];
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_AtrousFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_BlurTarget[(i + 1) & 1]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_VarianceTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);

		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	for (uint32_t i = 0; i < 2; ++i)
	{
		IKPipelinePtr& pipeline = m_ComposePipeline[i];
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_ComposeFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_BlurTarget[i]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_VarianceTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}
}

bool KScreenSpaceReflection::Init(uint32_t width, uint32_t height, float ratio)
{
	UnInit();

	m_Ratio = ratio;

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_HitResultTarget);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_HitMaskTarget);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_FinalTarget);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_FinalSquaredTarget);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_FinalTsppTarget);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_VarianceTarget);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_ComposeTarget);
	for (uint32_t i = 0; i < 2; ++i)
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_TemporalTarget[i]);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_TemporalSquaredTarget[i]);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_TemporalTsppTarget[i]);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_BlurTarget[i]);
		KRenderGlobal::RenderDevice->CreatePipeline(m_TemporalPipeline[i]);
		KRenderGlobal::RenderDevice->CreatePipeline(m_AtrousPipeline[i]);
		KRenderGlobal::RenderDevice->CreatePipeline(m_ComposePipeline[i]);
		KRenderGlobal::RenderDevice->CreateRenderPass(m_RayReusePass[i]);
		KRenderGlobal::RenderDevice->CreateRenderPass(m_BlitPass[i]);
		KRenderGlobal::RenderDevice->CreateRenderPass(m_AtrousPass[i]);
	}
	KRenderGlobal::RenderDevice->CreatePipeline(m_RayReusePipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_ReflectionPipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_BlitPipeline);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_ReflectionPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_TemporalPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_ComposePass);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "ssr/ssr.frag", m_ReflectionFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "ssr/ssr_reuse.frag", m_RayReuseFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "ssr/ssr_temporal.frag", m_TemporalFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "ssr/ssr_blit.frag", m_BlitFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "ssr/ssr_atrous.frag", m_AtrousFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "ssr/ssr_compose.frag", m_ComposeFS, false);

	KRenderGlobal::RenderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	KRenderGlobal::RenderDevice->CreateCommandBuffer(m_PrimaryCommandBuffer);
	m_PrimaryCommandBuffer->Init(m_CommandPool, CBL_PRIMARY);

	Resize(width, height);

	m_DebugDrawer.Init(m_ComposeTarget, 0, 0, 1, 1);

	return true;
}

bool KScreenSpaceReflection::UnInit()
{
	SAFE_UNINIT(m_PrimaryCommandBuffer);
	SAFE_UNINIT(m_CommandPool);

	m_DebugDrawer.UnInit();
	SAFE_UNINIT(m_HitResultTarget);
	SAFE_UNINIT(m_HitMaskTarget);
	SAFE_UNINIT(m_FinalTarget);
	SAFE_UNINIT(m_FinalSquaredTarget);
	SAFE_UNINIT(m_FinalTsppTarget);
	SAFE_UNINIT(m_VarianceTarget);
	SAFE_UNINIT(m_ComposeTarget);
	for (uint32_t i = 0; i < 2; ++i)
	{
		SAFE_UNINIT(m_TemporalTarget[i]);
		SAFE_UNINIT(m_TemporalSquaredTarget[i]);
		SAFE_UNINIT(m_TemporalTsppTarget[i]);
		SAFE_UNINIT(m_BlurTarget[i]);

		SAFE_UNINIT(m_TemporalPipeline[i]);
		SAFE_UNINIT(m_AtrousPipeline[i]);
		SAFE_UNINIT(m_ComposePipeline[i]);
		SAFE_UNINIT(m_RayReusePass[i]);
		SAFE_UNINIT(m_BlitPass[i]);
		SAFE_UNINIT(m_AtrousPass[i]);
	}
	SAFE_UNINIT(m_RayReusePipeline);
	SAFE_UNINIT(m_ReflectionPipeline);	
	SAFE_UNINIT(m_BlitPipeline);
	SAFE_UNINIT(m_ReflectionPass);
	SAFE_UNINIT(m_TemporalPass);
	SAFE_UNINIT(m_ComposePass);

	m_QuadVS.Release();
	m_ReflectionFS.Release();
	m_RayReuseFS.Release();
	m_TemporalFS.Release();
	m_BlitFS.Release();
	m_AtrousFS.Release();
	m_ComposeFS.Release();

	return true;
}

bool KScreenSpaceReflection::ReloadShader()
{
	if (m_QuadVS)
		m_QuadVS->Reload();
	if (m_ReflectionFS)
		m_ReflectionFS->Reload();
	if (m_RayReuseFS)
		m_RayReuseFS->Reload();
	if (m_TemporalFS)
		m_TemporalFS->Reload();
	if (m_BlitFS)
		m_BlitFS->Reload();
	if (m_AtrousFS)
		m_AtrousFS->Reload();
	if (m_ComposeFS)
		m_ComposeFS->Reload();
	if (m_ReflectionPipeline)
		m_ReflectionPipeline->Reload();
	if (m_RayReusePipeline)
		m_RayReusePipeline->Reload();
	if (m_BlitPipeline)
		m_BlitPipeline->Reload();
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (m_TemporalPipeline[i])
			m_TemporalPipeline[i]->Reload();
		if (m_AtrousPipeline[i])
			m_AtrousPipeline[i]->Reload();
		if (m_ComposePipeline[i])
			m_ComposePipeline[i]->Reload();
	}
	return true;
}

bool KScreenSpaceReflection::Resize(uint32_t width, uint32_t height)
{
	m_CurrentIdx = 0;
	m_FirstFrame = true;

	m_Width = std::max((uint32_t)(width * m_Ratio), 1U);
	m_Height = std::max((uint32_t)(height * m_Ratio), 1U);

	m_HitResultTarget->UnInit();
	m_HitResultTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

	m_HitMaskTarget->UnInit();
	m_HitMaskTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R8_UNORM);

	for (uint32_t i = 0; i < 2; ++i)
	{
		m_TemporalTarget[i]->UnInit();
		m_TemporalTarget[i]->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

		m_TemporalSquaredTarget[i]->UnInit();
		m_TemporalSquaredTarget[i]->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

		m_TemporalTsppTarget[i]->UnInit();
		m_TemporalTsppTarget[i]->InitFromColor(m_Width, m_Height, 1, 1, EF_R8_UNORM);

		m_BlurTarget[i]->UnInit();
		m_BlurTarget[i]->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);
	}

	m_FinalTarget->UnInit();
	m_FinalTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

	m_FinalSquaredTarget->UnInit();
	m_FinalSquaredTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

	m_FinalTsppTarget->UnInit();
	m_FinalTsppTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R8_UNORM);

	m_ComposeTarget->UnInit();
	m_ComposeTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

	m_VarianceTarget->UnInit();
	m_VarianceTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

	m_ReflectionPass->UnInit();
	m_ReflectionPass->SetColorAttachment(0, m_HitResultTarget->GetFrameBuffer());
	m_ReflectionPass->SetColorAttachment(1, m_HitMaskTarget->GetFrameBuffer());
	m_ReflectionPass->SetOpColor(0, LO_CLEAR, SO_STORE);
	m_ReflectionPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_ReflectionPass->SetOpColor(1, LO_CLEAR, SO_STORE);
	m_ReflectionPass->SetClearColor(1, { 0.0f, 0.0f, 0.0f, 0.0f });
	ASSERT_RESULT(m_ReflectionPass->Init());

	for (uint32_t i = 0; i < 2; ++i)
	{
		m_RayReusePass[i]->UnInit();
		m_RayReusePass[i]->SetColorAttachment(0, m_TemporalTarget[i]->GetFrameBuffer());
		m_RayReusePass[i]->SetOpColor(0, LO_CLEAR, SO_STORE);
		m_RayReusePass[i]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		ASSERT_RESULT(m_RayReusePass[i]->Init());
	}

	m_TemporalPass->UnInit();
	m_TemporalPass->SetColorAttachment(0, m_FinalTarget->GetFrameBuffer());
	m_TemporalPass->SetOpColor(0, LO_CLEAR, SO_STORE);
	m_TemporalPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });

	m_TemporalPass->SetColorAttachment(1, m_FinalSquaredTarget->GetFrameBuffer());
	m_TemporalPass->SetOpColor(1, LO_CLEAR, SO_STORE);
	m_TemporalPass->SetClearColor(1, { 0.0f, 0.0f, 0.0f, 0.0f });

	m_TemporalPass->SetColorAttachment(2, m_FinalTsppTarget->GetFrameBuffer());
	m_TemporalPass->SetOpColor(2, LO_CLEAR, SO_STORE);
	m_TemporalPass->SetClearColor(2, { 0.0f, 0.0f, 0.0f, 0.0f });

	m_TemporalPass->SetColorAttachment(3, m_VarianceTarget->GetFrameBuffer());
	m_TemporalPass->SetOpColor(3, LO_CLEAR, SO_STORE);
	m_TemporalPass->SetClearColor(3, { 0.0f, 0.0f, 0.0f, 0.0f });

	ASSERT_RESULT(m_TemporalPass->Init());

	for (uint32_t i = 0; i < 2; ++i)
	{
		m_BlitPass[i]->UnInit();

		m_BlitPass[i]->SetColorAttachment(0, m_TemporalTarget[i]->GetFrameBuffer());
		m_BlitPass[i]->SetOpColor(0, LO_CLEAR, SO_STORE);
		m_BlitPass[i]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });

		m_BlitPass[i]->SetColorAttachment(1, m_TemporalSquaredTarget[i]->GetFrameBuffer());
		m_BlitPass[i]->SetOpColor(1, LO_CLEAR, SO_STORE);
		m_BlitPass[i]->SetClearColor(1, { 0.0f, 0.0f, 0.0f, 0.0f });

		m_BlitPass[i]->SetColorAttachment(2, m_TemporalTsppTarget[i]->GetFrameBuffer());
		m_BlitPass[i]->SetOpColor(2, LO_CLEAR, SO_STORE);
		m_BlitPass[i]->SetClearColor(2, { 0.0f, 0.0f, 0.0f, 0.0f });

		m_BlitPass[i]->SetColorAttachment(3, m_BlurTarget[i]->GetFrameBuffer());
		m_BlitPass[i]->SetOpColor(3, LO_CLEAR, SO_STORE);
		m_BlitPass[i]->SetClearColor(3, { 0.0f, 0.0f, 0.0f, 0.0f });

		ASSERT_RESULT(m_BlitPass[i]->Init());
	}

	for (uint32_t i = 0; i < 2; ++i)
	{
		m_AtrousPass[i]->UnInit();
		m_AtrousPass[i]->SetColorAttachment(0, m_BlurTarget[i]->GetFrameBuffer());
		m_AtrousPass[i]->SetOpColor(0, LO_CLEAR, SO_STORE);
		m_AtrousPass[i]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		ASSERT_RESULT(m_AtrousPass[i]->Init());
	}

	m_ComposePass->UnInit();
	m_ComposePass->SetColorAttachment(0, m_ComposeTarget->GetFrameBuffer());
	m_ComposePass->SetOpColor(0, LO_CLEAR, SO_STORE);
	m_ComposePass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	ASSERT_RESULT(m_ComposePass->Init());

	InitializePipeline();

	m_PrimaryCommandBuffer->BeginPrimary();

	for (uint32_t i = 0; i < 2; ++i)
	{
		m_PrimaryCommandBuffer->BeginRenderPass(m_BlitPass[i], SUBPASS_CONTENTS_INLINE);
		m_PrimaryCommandBuffer->SetViewport(m_BlitPass[i]->GetViewPort());
		m_PrimaryCommandBuffer->EndRenderPass();
		m_PrimaryCommandBuffer->Translate(m_TemporalTarget[i]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		m_PrimaryCommandBuffer->Translate(m_TemporalSquaredTarget[i]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		m_PrimaryCommandBuffer->Translate(m_TemporalTsppTarget[i]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		m_PrimaryCommandBuffer->Translate(m_BlurTarget[i]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	m_PrimaryCommandBuffer->BeginRenderPass(m_TemporalPass, SUBPASS_CONTENTS_INLINE);
	m_PrimaryCommandBuffer->SetViewport(m_TemporalPass->GetViewPort());
	m_PrimaryCommandBuffer->EndRenderPass();
	m_PrimaryCommandBuffer->Translate(m_FinalTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	m_PrimaryCommandBuffer->BeginRenderPass(m_ComposePass, SUBPASS_CONTENTS_INLINE);
	m_PrimaryCommandBuffer->SetViewport(m_ComposePass->GetViewPort());
	m_PrimaryCommandBuffer->EndRenderPass();
	m_PrimaryCommandBuffer->Translate(m_ComposeTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	m_PrimaryCommandBuffer->End();
	m_PrimaryCommandBuffer->Flush();

	return true;
}

bool KScreenSpaceReflection::DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	return m_DebugDrawer.Render(renderPass, primaryBuffer);
}

bool KScreenSpaceReflection::Execute(IKCommandBufferPtr primaryBuffer)
{
	if (m_FirstFrame)
	{
		m_FirstFrame = false;
		return true;
	}

	primaryBuffer->BeginDebugMarker("SSR", glm::vec4(0, 1, 0, 0));

	{
		primaryBuffer->BeginDebugMarker("SSR_Trace", glm::vec4(0, 1, 0, 0));
		primaryBuffer->Translate(KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->BeginRenderPass(m_ReflectionPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_ReflectionPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		struct ObjectData
		{
			int32_t maxHiZMip;
			uint32_t frameNum;
		} objectData;
		objectData.maxHiZMip = (int32_t)KRenderGlobal::HiZBuffer.GetNumMips() - 1;
		objectData.frameNum = KRenderGlobal::CurrentFrameNum;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		command.objectUsage = objectUsage;

		command.pipeline = m_ReflectionPipeline;
		command.pipeline->GetHandle(m_ReflectionPass, command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Translate(m_HitResultTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(m_HitMaskTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->Translate(KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("SSR_RayReuse", glm::vec4(0, 1, 0, 0));
		primaryBuffer->Translate(KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->BeginRenderPass(m_RayReusePass[m_CurrentIdx], SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_RayReusePass[m_CurrentIdx]->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		struct ObjectData
		{
		} objectData;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		// command.objectUsage = objectUsage;

		command.pipeline = m_RayReusePipeline;
		command.pipeline->GetHandle(m_RayReusePass[m_CurrentIdx], command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Translate(m_TemporalTarget[m_CurrentIdx]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->Translate(KRenderGlobal::DeferredRenderer.GetSceneColor()->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("SSR_Temporal", glm::vec4(0, 1, 0, 0));

		primaryBuffer->BeginRenderPass(m_TemporalPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_TemporalPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		struct ObjectData
		{
		} objectData;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		// command.objectUsage = objectUsage;

		command.pipeline = m_TemporalPipeline[m_CurrentIdx];
		command.pipeline->GetHandle(m_TemporalPass, command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Translate(m_FinalTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(m_FinalSquaredTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(m_FinalTsppTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(m_VarianceTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("SSR_Blit", glm::vec4(0, 1, 0, 0));

		primaryBuffer->BeginRenderPass(m_BlitPass[m_CurrentIdx], SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_BlitPass[m_CurrentIdx]->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		command.pipeline = m_BlitPipeline;
		command.pipeline->GetHandle(m_BlitPass[m_CurrentIdx], command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Translate(m_TemporalTarget[m_CurrentIdx]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(m_TemporalSquaredTarget[m_CurrentIdx]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(m_TemporalTsppTarget[m_CurrentIdx]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(m_BlurTarget[m_CurrentIdx]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("SSR_Atrous", glm::vec4(0, 1, 0, 0));
		for (int32_t level = 1; level <= m_AtrousLevel; ++level)
		{
			primaryBuffer->BeginDebugMarker("SSR_AtrousPass" + std::to_string(level), glm::vec4(0, 1, 0, 0));

			uint32_t i = level & 1;

			primaryBuffer->BeginRenderPass(m_AtrousPass[i], SUBPASS_CONTENTS_INLINE);
			primaryBuffer->SetViewport(m_AtrousPass[i]->GetViewPort());

			KRenderCommand command;
			command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
			command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
			command.indexDraw = true;

			struct ObjectData
			{
				uint32_t level;
			} objectData;
			objectData.level = level;

			KDynamicConstantBufferUsage objectUsage;
			objectUsage.binding = SHADER_BINDING_OBJECT;
			objectUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

			command.objectUsage = objectUsage;

			command.pipeline = m_AtrousPipeline[i];
			command.pipeline->GetHandle(m_AtrousPass[i], command.pipelineHandle);

			primaryBuffer->Render(command);

			primaryBuffer->EndRenderPass();

			primaryBuffer->Translate(m_BlurTarget[i]->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

			primaryBuffer->EndDebugMarker();
		}
		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("SSR_Compose", glm::vec4(0, 1, 0, 0));

		uint32_t i = m_AtrousLevel & 1;

		primaryBuffer->BeginRenderPass(m_ComposePass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_ComposePass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		command.pipeline = m_ComposePipeline[i];
		command.pipeline->GetHandle(m_ComposePass, command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Translate(m_ComposeTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->EndDebugMarker();
	}

	primaryBuffer->EndDebugMarker();

	m_CurrentIdx ^= 1;

	return true;
}