#include "KDepthOfField.h"
#include "Internal/KRenderGlobal.h"

KDepthOfField::KDepthOfField()
	: m_CoCMax(0.2f)
	, m_CoCLimitRatio(0.95f)
	// , m_Aperture(10.0f)
	, m_FStop(1.0f)
	, m_FocusDistance(200.0f)
	, m_FocalLength(1.0f)
	, m_FarRange(2500.0f)
	, m_NearRange(90.0f)
	, m_Ratio(0.5f)
	, m_InputWidth(0)
	, m_InputHeight(0)
	, m_Width(0)
	, m_Height(0)
{
}

KDepthOfField::~KDepthOfField()
{
}

bool KDepthOfField::Init(uint32_t width, uint32_t height, float ratio)
{
	m_Ratio = ratio;

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_CoC);

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_Red);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_Green);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_Blue);

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_Final);

	KRenderGlobal::RenderDevice->CreateRenderPass(m_CoCPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_HorizontalPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_VerticalPass);

	KRenderGlobal::RenderDevice->CreatePipeline(m_CoCPipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_HorizontalPipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_VerticalPipeline);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/coc.frag", m_CocFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/horizontal.frag", m_HorizontalFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/vertical.frag", m_VerticalFS, false);

	Resize(width, height);

	m_DebugDrawer.Init(m_Final, 0, 0, 1, 1);

	return true;
}

bool KDepthOfField::UnInit()
{
	m_DebugDrawer.UnInit();

	SAFE_UNINIT(m_Final);

	SAFE_UNINIT(m_Red);
	SAFE_UNINIT(m_Green);
	SAFE_UNINIT(m_Blue);

	SAFE_UNINIT(m_CoC);

	SAFE_UNINIT(m_CoCPipeline);
	SAFE_UNINIT(m_HorizontalPipeline);
	SAFE_UNINIT(m_VerticalPipeline);

	SAFE_UNINIT(m_CoCPass);
	SAFE_UNINIT(m_HorizontalPass);
	SAFE_UNINIT(m_VerticalPass);

	m_QuadVS.Release();
	m_CocFS.Release();
	m_HorizontalFS.Release();
	m_VerticalFS.Release();

	return true;
}

void KDepthOfField::InitializePipeline()
{
	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);

	{
		IKPipelinePtr& pipeline = m_CoCPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_CocFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_HorizontalPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_HorizontalFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_CoC->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_VerticalPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_VerticalFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_Red->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE3, m_Green->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE4, m_Blue->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE5, m_CoC->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}
}

bool KDepthOfField::ReloadShader()
{
	if(m_QuadVS)
		m_QuadVS->Reload();
	if (m_CocFS)
		m_CocFS->Reload();
	if (m_HorizontalFS)
		m_HorizontalFS->Reload();
	if (m_VerticalFS)
		m_VerticalFS->Reload();
	if (m_CoCPipeline)
		m_CoCPipeline->Reload();
	if (m_HorizontalPipeline)
		m_HorizontalPipeline->Reload();
	if (m_VerticalPipeline)
		m_VerticalPipeline->Reload();
	return true;
}

bool KDepthOfField::Resize(uint32_t width, uint32_t height)
{
	m_InputWidth = width;
	m_InputHeight = height;

	m_Width = std::max((uint32_t)(width * m_Ratio), 1U);
	m_Height = std::max((uint32_t)(height * m_Ratio), 1U);

	m_CoC->UnInit();
	m_CoC->InitFromColor(m_Width, m_Height, 1, 1, EF_R8_UNORM);

	m_Red->UnInit();
	m_Red->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);
	m_Green->UnInit();
	m_Green->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);
	m_Blue->UnInit();
	m_Blue->InitFromColor(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

	m_Final->UnInit();
	m_Final->InitFromColor(m_InputWidth, m_InputHeight, 1, 1, EF_R8G8B8A8_UNORM);

	m_CoCPass->UnInit();
	m_CoCPass->SetColorAttachment(0, m_CoC->GetFrameBuffer());
	m_CoCPass->SetOpColor(0, LO_DONT_CARE, SO_STORE);
	m_CoCPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_CoCPass->Init();

	m_HorizontalPass->UnInit();
	m_HorizontalPass->SetColorAttachment(0, m_Red->GetFrameBuffer());
	m_HorizontalPass->SetOpColor(0, LO_DONT_CARE, SO_STORE);
	m_HorizontalPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_HorizontalPass->Init();
	m_HorizontalPass->SetColorAttachment(1, m_Green->GetFrameBuffer());
	m_HorizontalPass->SetOpColor(1, LO_DONT_CARE, SO_STORE);
	m_HorizontalPass->SetClearColor(1, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_HorizontalPass->Init();
	m_HorizontalPass->SetColorAttachment(2, m_Blue->GetFrameBuffer());
	m_HorizontalPass->SetOpColor(2, LO_DONT_CARE, SO_STORE);
	m_HorizontalPass->SetClearColor(2, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_HorizontalPass->Init();

	m_VerticalPass->UnInit();
	m_VerticalPass->SetColorAttachment(0, m_Final->GetFrameBuffer());
	m_VerticalPass->SetOpColor(0, LO_DONT_CARE, SO_STORE);
	m_VerticalPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_VerticalPass->Init();

	InitializePipeline();

	return true;
}

bool KDepthOfField::DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	return m_DebugDrawer.Render(renderPass, primaryBuffer);
}

float KDepthOfField::CalcCoC(float distance)
{
	float aperture = m_FocalLength / m_FStop;
	float coc = aperture * abs(distance - m_FocusDistance) * m_FocalLength / (distance * (m_FocusDistance - m_FocalLength));
	return coc;
}

// https://zh.wikipedia.org/wiki/%E6%99%AF%E6%B7%B1
float KDepthOfField::CalcDofNear(float C)
{
	float numerator = m_FocusDistance * m_FocalLength * m_FocalLength;
	float denominator = m_FocalLength * m_FocalLength + m_FStop * C * (m_FocusDistance - m_FocalLength);
	return numerator / denominator;
}

float KDepthOfField::CalcDofFar(float C)
{
	float numerator = m_FocusDistance * m_FocalLength * m_FocalLength;
	float denominator = m_FocalLength * m_FocalLength - m_FStop * C * (m_FocusDistance - m_FocalLength);
	return numerator / denominator;
}

bool KDepthOfField::Execute(IKCommandBufferPtr primaryBuffer)
{
	float aperture = m_FocalLength / m_FStop;

	m_CoCLimitRatio = std::min(1.0f - 1e-3f, m_CoCLimitRatio);
	m_FocalLength = std::max(0.001f, std::min(m_FocusDistance - 1.0f, m_FocalLength));
	m_NearRange = std::max(0.001f, std::min(m_FocusDistance - 1.0f, m_NearRange));

	// Furthest CoC
	float furthestCoC = aperture * m_FocalLength / (m_FocusDistance - m_FocalLength);

	float N = std::max(1e-3f, m_FocusDistance - m_NearRange);
	float F = m_FocusDistance + m_FarRange;

	float DOF = 0.0f;
	float C = 0.0f;

	// Pick the smallest CoC
	m_CoCMax = std::min(furthestCoC * 0.98f, CalcCoC(F));
	C = m_CoCMax * m_CoCLimitRatio;
	N = CalcDofNear(C);
	F = CalcDofFar(C);
	DOF = F - N;

	float cocNear = CalcCoC(N);
	float cocFar = CalcCoC(F);
	float cocFocus = CalcCoC(m_FocusDistance);

	float eps = exp2f((float)KMath::MantissaCount(C)) * KMath::FloatPrecision(C);
	ASSERT_RESULT(abs(cocNear - C) < eps && "Mathematical must match exactly");
	ASSERT_RESULT(abs(cocFar - C) < eps && "Mathematical must match exactly");
	ASSERT_RESULT(abs(cocFocus) < 1e-5f && "Mathematical must match exactly");

	struct ObjectData
	{
		// Aperture FocusDistance FocalLength CoCMax
		glm::vec4 dofParams;
		// CoCLimitRatio NearDof FarDof MaxRadius
		glm::vec4 dofParams2;
	} objectData;

	objectData.dofParams[0] = aperture;
	objectData.dofParams[1] = m_FocusDistance;
	objectData.dofParams[2] = m_FocalLength;
	objectData.dofParams[3] = m_CoCMax;

	objectData.dofParams2[0] = m_CoCLimitRatio;
	objectData.dofParams2[1] = N;
	objectData.dofParams2[2] = F;
	objectData.dofParams2[3] = 1.0f;

	primaryBuffer->BeginDebugMarker("DOF", glm::vec4(1));
	primaryBuffer->Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	primaryBuffer->Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	{
		primaryBuffer->BeginDebugMarker("DOF_CoC", glm::vec4(1));

		primaryBuffer->BeginRenderPass(m_CoCPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_CoCPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		command.dynamicConstantUsages.push_back(objectUsage);

		command.pipeline = m_CoCPipeline;
		command.pipeline->GetHandle(m_CoCPass, command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Transition(m_CoC->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("DOF_Horizontal", glm::vec4(1));

		primaryBuffer->BeginRenderPass(m_HorizontalPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_HorizontalPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		command.dynamicConstantUsages.push_back(objectUsage);

		command.pipeline = m_HorizontalPipeline;
		command.pipeline->GetHandle(m_HorizontalPass, command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Transition(m_Red->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Transition(m_Green->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Transition(m_Blue->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->EndDebugMarker();
	}

	{
		primaryBuffer->BeginDebugMarker("DOF_Vertical", glm::vec4(1));

		primaryBuffer->BeginRenderPass(m_VerticalPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(m_VerticalPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		command.dynamicConstantUsages.push_back(objectUsage);

		command.pipeline = m_VerticalPipeline;
		command.pipeline->GetHandle(m_VerticalPass, command.pipelineHandle);

		primaryBuffer->Render(command);

		primaryBuffer->EndRenderPass();

		primaryBuffer->Transition(m_Final->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->EndDebugMarker();
	}
	primaryBuffer->Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	primaryBuffer->Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	primaryBuffer->EndDebugMarker();

	return true;
}