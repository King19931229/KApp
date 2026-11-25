#include "KDepthOfField.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"
#include <cmath>

KDepthOfField::KDepthOfField()
	: m_CoCLimitRatio(0.0f)
	// , m_Aperture(10.0f)
	, m_FStop(1.0f)
	, m_FocusDistance(200.0f)
	, m_FocalLength(1.0f)
	, m_FarRange(2500.0f)
	, m_Ratio(0.5f)
	, m_InputWidth(0)
	, m_InputHeight(0)
	, m_Width(0)
	, m_Height(0)
	, m_NeedInitialTransition(true)
	, m_InvalidParamsLogged(false)
{
}

KDepthOfField::~KDepthOfField()
{
}

bool KDepthOfField::Init(uint32_t width, uint32_t height, float ratio)
{
	if (std::abs(ratio - 0.5f) > 1e-6f)
	{
		KG_LOGW(LM_RENDER, "DOF prefilter only supports a 0.5 working ratio");
	}
	m_Ratio = 0.5f;

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_NearColor);
	KRenderGlobal::RenderDevice->CreateRenderTarget(m_FarColor);
	for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(intermediate);
	}

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_Final);

	KRenderGlobal::RenderDevice->CreateRenderPass(m_PreprocessPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_NearHorizontalPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_NearVerticalPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_FarHorizontalPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_FarVerticalPass);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_CompositePass);

	KRenderGlobal::RenderDevice->CreatePipeline(m_PreprocessPipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_NearHorizontalPipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_NearVerticalPipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_FarHorizontalPipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_FarVerticalPipeline);
	KRenderGlobal::RenderDevice->CreatePipeline(m_CompositePipeline);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/preprocess.frag", m_PreprocessFS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/horizontal.frag", m_NearHorizontalFS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/vertical.frag", m_NearVerticalFS, KRenderGlobal::EnableAsyncShaderCompile);

	KShaderCompileEnvironment farEnv;
	farEnv.macros.push_back({ "DOF_FAR", "1" });
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/horizontal.frag", farEnv, m_FarHorizontalFS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/vertical.frag", farEnv, m_FarVerticalFS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/composite.frag", m_CompositeFS, KRenderGlobal::EnableAsyncShaderCompile);

	Resize(width, height);

	return true;
}

bool KDepthOfField::UnInit()
{
	m_DebugDrawer.UnInit();

	SAFE_UNINIT(m_Final);

	SAFE_UNINIT(m_FarColor);
	SAFE_UNINIT(m_NearColor);
	for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
	{
		SAFE_UNINIT(intermediate);
	}

	SAFE_UNINIT(m_PreprocessPipeline);
	SAFE_UNINIT(m_NearHorizontalPipeline);
	SAFE_UNINIT(m_NearVerticalPipeline);
	SAFE_UNINIT(m_FarHorizontalPipeline);
	SAFE_UNINIT(m_FarVerticalPipeline);
	SAFE_UNINIT(m_CompositePipeline);

	SAFE_UNINIT(m_PreprocessPass);
	SAFE_UNINIT(m_NearHorizontalPass);
	SAFE_UNINIT(m_NearVerticalPass);
	SAFE_UNINIT(m_FarHorizontalPass);
	SAFE_UNINIT(m_FarVerticalPass);
	SAFE_UNINIT(m_CompositePass);

	m_QuadVS.Release();
	m_PreprocessFS.Release();
	m_NearHorizontalFS.Release();
	m_NearVerticalFS.Release();
	m_FarHorizontalFS.Release();
	m_FarVerticalFS.Release();
	m_CompositeFS.Release();

	return true;
}

void KDepthOfField::InitializePipeline()
{
	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);

	{
		IKPipelinePtr& pipeline = m_PreprocessPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_PreprocessFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_NearHorizontalPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_NearHorizontalFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_NearColor->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_NearVerticalPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_NearVerticalFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_ComplexIntermediate[0]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_ComplexIntermediate[1]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_FarHorizontalPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_FarHorizontalFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_FarColor->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_FarVerticalPipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_FarVerticalFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_ComplexIntermediate[0]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_ComplexIntermediate[1]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_ComplexIntermediate[2]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE3, m_ComplexIntermediate[3]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = m_CompositePipeline;
		pipeline->UnInit();

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_CompositeFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_NearColor->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE3, m_FarColor->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), false);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}
}

bool KDepthOfField::Reload()
{
	if(m_QuadVS)
		m_QuadVS->Reload();
	if (m_PreprocessFS)
		m_PreprocessFS->Reload();
	if (m_NearHorizontalFS)
		m_NearHorizontalFS->Reload();
	if (m_NearVerticalFS)
		m_NearVerticalFS->Reload();
	if (m_FarHorizontalFS)
		m_FarHorizontalFS->Reload();
	if (m_FarVerticalFS)
		m_FarVerticalFS->Reload();
	if (m_CompositeFS)
		m_CompositeFS->Reload();
	if (m_PreprocessPipeline)
		m_PreprocessPipeline->Reload(false);
	if (m_NearHorizontalPipeline)
		m_NearHorizontalPipeline->Reload(false);
	if (m_NearVerticalPipeline)
		m_NearVerticalPipeline->Reload(false);
	if (m_FarHorizontalPipeline)
		m_FarHorizontalPipeline->Reload(false);
	if (m_FarVerticalPipeline)
		m_FarVerticalPipeline->Reload(false);
	if (m_CompositePipeline)
		m_CompositePipeline->Reload(false);
	return true;
}

bool KDepthOfField::Resize(uint32_t width, uint32_t height)
{
	m_DebugDrawer.UnInit();

	m_InputWidth = width;
	m_InputHeight = height;

	m_Width = std::max((uint32_t)(width * m_Ratio + 0.5f), 1U);
	m_Height = std::max((uint32_t)(height * m_Ratio + 0.5f), 1U);
	m_NeedInitialTransition = true;

	auto InitColorTarget = [](IKRenderTargetPtr& target, uint32_t targetWidth, uint32_t targetHeight)
	{
		target->UnInit();
		target->InitFromColor(targetWidth, targetHeight, 1, 1, PF_R16G16B16A16_FLOAT);
	};

	InitColorTarget(m_NearColor, m_Width, m_Height);
	InitColorTarget(m_FarColor, m_Width, m_Height);
	for (uint32_t i = 0; i < ARRAY_SIZE(m_ComplexIntermediate); ++i)
	{
		InitColorTarget(m_ComplexIntermediate[i], m_Width, m_Height);
	}

	InitColorTarget(m_Final, m_InputWidth, m_InputHeight);

	m_PreprocessPass->UnInit();
	m_PreprocessPass->SetColorAttachment(0, m_NearColor->GetFrameBuffer());
	m_PreprocessPass->SetOpColor(0, LO_DONT_CARE, SO_STORE);
	m_PreprocessPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_PreprocessPass->SetColorAttachment(1, m_FarColor->GetFrameBuffer());
	m_PreprocessPass->SetOpColor(1, LO_DONT_CARE, SO_STORE);
	m_PreprocessPass->SetClearColor(1, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_PreprocessPass->Init();

	m_NearHorizontalPass->UnInit();
	m_NearHorizontalPass->SetColorAttachment(0, m_ComplexIntermediate[0]->GetFrameBuffer());
	m_NearHorizontalPass->SetOpColor(0, LO_DONT_CARE, SO_STORE);
	m_NearHorizontalPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_NearHorizontalPass->SetColorAttachment(1, m_ComplexIntermediate[1]->GetFrameBuffer());
	m_NearHorizontalPass->SetOpColor(1, LO_DONT_CARE, SO_STORE);
	m_NearHorizontalPass->SetClearColor(1, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_NearHorizontalPass->Init();

	m_NearVerticalPass->UnInit();
	m_NearVerticalPass->SetColorAttachment(0, m_NearColor->GetFrameBuffer());
	m_NearVerticalPass->SetOpColor(0, LO_DONT_CARE, SO_STORE);
	m_NearVerticalPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_NearVerticalPass->Init();

	m_FarHorizontalPass->UnInit();
	for (uint32_t i = 0; i < ARRAY_SIZE(m_ComplexIntermediate); ++i)
	{
		m_FarHorizontalPass->SetColorAttachment(i, m_ComplexIntermediate[i]->GetFrameBuffer());
		m_FarHorizontalPass->SetOpColor(i, LO_DONT_CARE, SO_STORE);
		m_FarHorizontalPass->SetClearColor(i, { 0.0f, 0.0f, 0.0f, 0.0f });
	}
	m_FarHorizontalPass->Init();

	m_FarVerticalPass->UnInit();
	m_FarVerticalPass->SetColorAttachment(0, m_FarColor->GetFrameBuffer());
	m_FarVerticalPass->SetOpColor(0, LO_DONT_CARE, SO_STORE);
	m_FarVerticalPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_FarVerticalPass->Init();

	m_CompositePass->UnInit();
	m_CompositePass->SetColorAttachment(0, m_Final->GetFrameBuffer());
	m_CompositePass->SetOpColor(0, LO_DONT_CARE, SO_STORE);
	m_CompositePass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_CompositePass->Init();

	InitializePipeline();

	m_DebugDrawer.Init(m_Final->GetFrameBuffer(), 0, 0, 1, 1);

	return true;
}

bool KDepthOfField::DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	return m_DebugDrawer.Render(renderPass, commandList);
}

float KDepthOfField::CalcCoC(float distance)
{
	float aperture = m_FocalLength / m_FStop;
	return aperture * abs(distance - m_FocusDistance) * m_FocalLength / (distance * (m_FocusDistance - m_FocalLength));
}

bool KDepthOfField::Execute(KRHICommandList& commandList)
{
	bool validParams =
		std::isfinite(m_FStop) && m_FStop > 0.0f &&
		std::isfinite(m_FocusDistance) && std::isfinite(m_FocalLength) &&
		std::isfinite(m_FarRange) && std::isfinite(m_CoCLimitRatio) &&
		m_FocalLength > 0.0f && m_FocusDistance > m_FocalLength && m_FarRange >= 0.0f;

	float aperture = 0.0f;
	float focusDistance = 0.0f;
	float focalLength = 0.0f;
	float cocMin = 0.0f;
	float cocMax = 0.0f;

	if (validParams)
	{
		float cocLimitRatio = std::min(1.0f - 1e-3f, std::max(0.0f, m_CoCLimitRatio));
		aperture = m_FocalLength / m_FStop;
		float furthestCoC = aperture * m_FocalLength / (m_FocusDistance - m_FocalLength);
		float farDistance = m_FocusDistance + m_FarRange;
		cocMax = std::min(furthestCoC * 0.98f, CalcCoC(farDistance));
		cocMin = cocMax * cocLimitRatio;
		focusDistance = m_FocusDistance;
		focalLength = m_FocalLength;
		validParams = std::isfinite(cocMax) && std::isfinite(cocMin) && cocMax > cocMin;
	}

	if (validParams)
	{
		m_InvalidParamsLogged = false;
	}
	else
	{
		aperture = focusDistance = focalLength = cocMin = cocMax = 0.0f;
		if (!m_InvalidParamsLogged)
		{
			KG_LOGW(LM_RENDER, "Invalid DOF parameters, disable blur until parameters are valid");
			m_InvalidParamsLogged = true;
		}
	}

	struct ObjectData
	{
		// Aperture FocusDistance FocalLength CoCMax
		glm::vec4 dofParams;
		// CoCMin Reserved Reserved Reserved
		glm::vec4 dofParams2;
	} objectData = {};

	objectData.dofParams = glm::vec4(aperture, focusDistance, focalLength, cocMax);
	objectData.dofParams2 = glm::vec4(cocMin, 0.0f, 0.0f, 0.0f);

	auto ToReadOnly = [&](IKRenderTargetPtr& target)
	{
		commandList.Transition(
			target->GetFrameBuffer(),
			PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
			PIPELINE_STAGE_FRAGMENT_SHADER,
			IMAGE_LAYOUT_COLOR_ATTACHMENT,
			IMAGE_LAYOUT_SHADER_READ_ONLY);
	};

	auto ToColorAttachment = [&](IKRenderTargetPtr& target)
	{
		commandList.Transition(
			target->GetFrameBuffer(),
			PIPELINE_STAGE_FRAGMENT_SHADER,
			PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
			IMAGE_LAYOUT_SHADER_READ_ONLY,
			IMAGE_LAYOUT_COLOR_ATTACHMENT);
	};

	auto DrawPass = [&](const char* marker, IKRenderPassPtr& pass, IKPipelinePtr& pipeline, bool bindDofParams)
	{
		commandList.BeginDebugMarker(marker, glm::vec4(1));
		commandList.BeginRenderPass(pass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(pass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

		if (bindDofParams)
		{
			KDynamicConstantBufferUsage objectUsage;
			objectUsage.binding = SHADER_BINDING_OBJECT;
			objectUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);
			command.dynamicConstantUsages.push_back(objectUsage);
		}

		command.pipeline = pipeline;
		command.pipeline->GetHandle(pass, command.pipelineHandle);
		commandList.Render(command);

		commandList.EndRenderPass();
		commandList.EndDebugMarker();
	};

	commandList.BeginDebugMarker("DOF", glm::vec4(1));
	commandList.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	if (m_NeedInitialTransition)
	{
		ToReadOnly(m_NearColor);
		ToReadOnly(m_FarColor);
		for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
		{
			ToReadOnly(intermediate);
		}
		ToReadOnly(m_Final);
		m_NeedInitialTransition = false;
	}

	ToColorAttachment(m_NearColor);
	ToColorAttachment(m_FarColor);
	DrawPass("DOF_Preprocess", m_PreprocessPass, m_PreprocessPipeline, true);
	ToReadOnly(m_NearColor);
	ToReadOnly(m_FarColor);

	ToColorAttachment(m_ComplexIntermediate[0]);
	ToColorAttachment(m_ComplexIntermediate[1]);
	DrawPass("DOF_NearHorizontal", m_NearHorizontalPass, m_NearHorizontalPipeline, false);
	ToReadOnly(m_ComplexIntermediate[0]);
	ToReadOnly(m_ComplexIntermediate[1]);

	ToColorAttachment(m_NearColor);
	DrawPass("DOF_NearVertical", m_NearVerticalPass, m_NearVerticalPipeline, false);
	ToReadOnly(m_NearColor);

	for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
	{
		ToColorAttachment(intermediate);
	}
	DrawPass("DOF_FarHorizontal", m_FarHorizontalPass, m_FarHorizontalPipeline, false);
	for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
	{
		ToReadOnly(intermediate);
	}

	ToColorAttachment(m_FarColor);
	DrawPass("DOF_FarVertical", m_FarVerticalPass, m_FarVerticalPipeline, false);
	ToReadOnly(m_FarColor);

	ToColorAttachment(m_Final);
	DrawPass("DOF_Composite", m_CompositePass, m_CompositePipeline, true);
	ToReadOnly(m_Final);

	commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	commandList.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	commandList.EndDebugMarker();

	return true;
}
