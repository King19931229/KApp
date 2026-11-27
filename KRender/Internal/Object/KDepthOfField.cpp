#include "KDepthOfField.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"
#include "Product/Shaders/dof/tile_config.h"
#include <cmath>

KDepthOfField::KDepthOfField()
	: m_FStop(1.0f)
	, m_FocusDistance(200.0f)
	, m_FocalLength(1.0f)
	, m_DebugView(DV_FINAL)
	, m_Enable(true)
	, m_NearTileEnable(false)
	, m_NearTileSupported(false)
	, m_WorkingResolution(0)
	, m_DownsampleFactor(2)
	, m_InputWidth(0)
	, m_InputHeight(0)
	, m_Width(0)
	, m_Height(0)
	, m_NeedInitialTransition(true)
	, m_FinalNeedsInitialTransition(true)
	, m_TileNeedsInitialTransition(true)
	, m_InvalidParamsLogged(false)
{
}

KDepthOfField::~KDepthOfField()
{
}

bool KDepthOfField::Init(uint32_t width, uint32_t height, float ratio)
{
	if (!width || !height)
	{
		KG_LOGW(LM_RENDER, "DOF requires a nonzero input resolution");
		return false;
	}
	m_DownsampleFactor = 2;
	if (std::isfinite(ratio) && std::abs(ratio - 0.25f) <= 1e-6f)
	{
		m_DownsampleFactor = 4;
	}
	else if (!std::isfinite(ratio) || std::abs(ratio - 0.5f) > 1e-6f)
	{
		KG_LOGW(LM_RENDER, "DOF supports working ratios 0.5 and 0.25; fall back to 0.5");
	}
	m_WorkingResolution = m_DownsampleFactor == 4 ? 1 : 0;
	KRenderDeviceProperties* properties = nullptr;
	m_NearTileSupported = KRenderGlobal::RenderDevice->QueryProperty(&properties) &&
		properties && properties->r16fStorageImageSupport;

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

	return Resize(width, height);
}

bool KDepthOfField::UnInit()
{
	// 与既有 UnInit 契约相同，调用方已进入安全销毁边界。
	ReleaseRetiredWorkingResources(true);
	m_LastFrameFence.reset();
	m_DebugDrawer.UnInit();
	SAFE_UNINIT(m_NearTileMaxPipeline);
	SAFE_UNINIT(m_NearTileHorizontalPipeline);
	SAFE_UNINIT(m_NearTileVerticalPipeline);
	SAFE_UNINIT(m_NearTileCoC);
	m_NearTileHorizontalFS.Release();
	m_NearTileVerticalFS.Release();

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
		// Tile 关闭时使用有效的占位采样器，诊断分支由 uniform 标志禁止访问。
		pipeline->SetSampler(SHADER_BINDING_TEXTURE4,
			(m_NearTileCoC ? m_NearTileCoC : m_NearColor)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		pipeline->Init();
	}
}

bool KDepthOfField::HasPendingResolutionChange() const
{
	return m_DownsampleFactor != (m_WorkingResolution == 1 ? 4U : 2U);
}

void KDepthOfField::RetiredWorkingResources::UnInit()
{
	SAFE_UNINIT(tilePipeline);
	for (auto& pipeline : pipelines)
	{
		// 本组独占 RenderPass 的最后使用 Fence 已完成，销毁 Handle 不应再等待整个设备。
		if (pipeline) pipeline->UnInit(true);
		pipeline.reset();
	}
	for (auto& pass : passes) SAFE_UNINIT(pass);
	for (auto& target : targets) SAFE_UNINIT(target);
}

void KDepthOfField::ReleaseRetiredWorkingResources(bool all)
{
	for (auto it = m_RetiredWorkingResources.begin(); it != m_RetiredWorkingResources.end();)
	{
		if (all || !it->completionFence || it->completionFence->GetStatus() == IKFence::Status::Signaled)
		{
			it->UnInit();
			it = m_RetiredWorkingResources.erase(it);
		}
		else ++it;
	}
}

void KDepthOfField::RetireWorkingResources()
{
	RetiredWorkingResources retired;
	retired.completionFence = std::move(m_LastFrameFence);
	// 旧描述符、Framebuffer 和 Image 必须一起保留，不能仅保留 RT 却原地 UnInit Pipeline。
	for (IKRenderTargetPtr* target : { &m_NearColor, &m_FarColor, &m_ComplexIntermediate[0],
		&m_ComplexIntermediate[1], &m_ComplexIntermediate[2], &m_ComplexIntermediate[3] })
	{
		retired.targets.push_back(std::move(*target));
		KRenderGlobal::RenderDevice->CreateRenderTarget(*target);
	}
	for (IKRenderPassPtr* pass : { &m_PreprocessPass, &m_NearHorizontalPass, &m_NearVerticalPass,
		&m_FarHorizontalPass, &m_FarVerticalPass, &m_CompositePass })
	{
		retired.passes.push_back(std::move(*pass));
		KRenderGlobal::RenderDevice->CreateRenderPass(*pass);
	}
	for (IKPipelinePtr* pipeline : { &m_PreprocessPipeline, &m_NearHorizontalPipeline, &m_NearVerticalPipeline,
		&m_FarHorizontalPipeline, &m_FarVerticalPipeline, &m_CompositePipeline })
	{
		retired.pipelines.push_back(std::move(*pipeline));
		KRenderGlobal::RenderDevice->CreatePipeline(*pipeline);
	}
	if (m_NearTileCoC)
	{
		retired.targets.push_back(std::move(m_NearTileCoC));
		retired.tilePipeline = std::move(m_NearTileMaxPipeline);
		retired.pipelines.push_back(std::move(m_NearTileHorizontalPipeline));
		retired.pipelines.push_back(std::move(m_NearTileVerticalPipeline));
	}
	m_RetiredWorkingResources.push_back(std::move(retired));
	ReleaseRetiredWorkingResources();
}

bool KDepthOfField::ApplySettings()
{
	if (!m_NearColor || !m_InputWidth || !m_InputHeight) return false;
	ReleaseRetiredWorkingResources();
	m_WorkingResolution = m_WorkingResolution == 1 ? 1 : 0;
	if (HasPendingResolutionChange())
	{
		const bool hadTile = bool(m_NearTileCoC);
		RetireWorkingResources();
		m_DownsampleFactor = m_WorkingResolution == 1 ? 4 : 2;
		if (!Resize(m_InputWidth, m_InputHeight)) return false;
		if (hadTile && !InitializeNearTile()) return false;
	}
	if (m_NearTileEnable && !m_NearTileCoC)
	{
		if (!InitializeNearTile())
		{
			m_NearTileEnable = false;
			KG_LOGW(LM_RENDER, "Near TileMAX unavailable; use the baseline convolution");
			return false;
		}
	}
	return true;
}

bool KDepthOfField::InitializeNearTile()
{
	if (!m_NearTileSupported) return false;
	if (!m_NearTileCoC) KRenderGlobal::RenderDevice->CreateRenderTarget(m_NearTileCoC);
	// 共享 Tile 尺寸由同一头文件提供给 C++ 和 GLSL，避免 CPU 分配与 Compute 分组分叉。
	const uint32_t tileWidth = (m_Width + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
	const uint32_t tileHeight = (m_Height + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
	if (!m_NearTileCoC->InitFromStorage(tileWidth, tileHeight, 1, PF_R16_FLOAT, true)) return false;
	m_NearTileCoC->GetFrameBuffer()->SetDebugName("DOF_NearTileCoC");
	m_TileNeedsInitialTransition = true;

	KShaderCompileEnvironment tileEnvironment;
	tileEnvironment.macros.push_back({"DOF_NEAR_TILE", "1"});
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/horizontal.frag", tileEnvironment, m_NearTileHorizontalFS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/vertical.frag", tileEnvironment, m_NearTileVerticalFS, KRenderGlobal::EnableAsyncShaderCompile);
	if (!m_NearTileMaxPipeline) KRenderGlobal::RenderDevice->CreateComputePipeline(m_NearTileMaxPipeline);
	m_NearTileMaxPipeline->UnInit();
	m_NearTileMaxPipeline->BindSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
	m_NearTileMaxPipeline->BindStorageImage(SHADER_BINDING_TEXTURE1, m_NearTileCoC->GetFrameBuffer(), PF_R16_FLOAT, COMPUTE_RESOURCE_OUT, 0, true);
	m_NearTileMaxPipeline->BindUniformBuffer(CBT_CAMERA, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA));
	m_NearTileMaxPipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	if (!m_NearTileMaxPipeline->Init("dof/near_tile_max.comp", KShaderCompileEnvironment(), false)) return false;

	auto InitializeBlur = [&](IKPipelinePtr& pipeline, const KShaderRef& fragment, bool vertical)
	{
		if (!pipeline) KRenderGlobal::RenderDevice->CreatePipeline(pipeline);
		pipeline->UnInit();
		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *fragment);
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, (vertical ? m_ComplexIntermediate[0] : m_NearColor)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
		if (vertical) pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_ComplexIntermediate[1]->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, m_NearTileCoC->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
		pipeline->Init();
	};
	InitializeBlur(m_NearTileHorizontalPipeline, m_NearTileHorizontalFS, false);
	InitializeBlur(m_NearTileVerticalPipeline, m_NearTileVerticalFS, true);
	m_CompositePipeline->SetSampler(SHADER_BINDING_TEXTURE4, m_NearTileCoC->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
	return true;
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
	if (m_NearTileHorizontalFS) m_NearTileHorizontalFS->Reload();
	if (m_NearTileVerticalFS) m_NearTileVerticalFS->Reload();
	if (m_NearTileHorizontalPipeline) m_NearTileHorizontalPipeline->Reload(false);
	if (m_NearTileVerticalPipeline) m_NearTileVerticalPipeline->Reload(false);
	if (m_NearTileMaxPipeline) m_NearTileMaxPipeline->Reload();
	return true;
}

bool KDepthOfField::Resize(uint32_t width, uint32_t height)
{
	// 拒绝零尺寸并保留现有资源；窗口恢复后由下一次有效 Resize 重新建立布局。
	if (!width || !height)
	{
		KG_LOGW(LM_RENDER, "Ignore zero-sized DOF resize");
		return false;
	}
	const bool resizeFinal = !m_InputWidth || !m_InputHeight || width != m_InputWidth || height != m_InputHeight ||
		!m_Final->GetFrameBuffer()->GetWidth();
	if (resizeFinal) m_DebugDrawer.UnInit();

	m_InputWidth = width;
	m_InputHeight = height;

	// 无加法溢出的向上取整，奇数尺寸最后一行/列仍参与预处理。
	m_Width = width / m_DownsampleFactor + (width % m_DownsampleFactor != 0);
	m_Height = height / m_DownsampleFactor + (height % m_DownsampleFactor != 0);
	m_NeedInitialTransition = true;

	auto InitColorTarget = [](IKRenderTargetPtr& target, uint32_t targetWidth, uint32_t targetHeight)
	{
		target->UnInit();
		// 初始布局转换放入正常帧命令，避开既有单次提交辅助函数中的 vkWaitForFences。
		target->InitFromColor(targetWidth, targetHeight, 1, 1, PF_R16G16B16A16_FLOAT, true);
	};

	InitColorTarget(m_NearColor, m_Width, m_Height);
	InitColorTarget(m_FarColor, m_Width, m_Height);
	for (uint32_t i = 0; i < ARRAY_SIZE(m_ComplexIntermediate); ++i)
	{
		InitColorTarget(m_ComplexIntermediate[i], m_Width, m_Height);
	}

	// 切换工作比例时保留 Final，现有下游采样绑定和全分辨率图像保持稳定。
	if (resizeFinal)
	{
		InitColorTarget(m_Final, m_InputWidth, m_InputHeight);
		m_FinalNeedsInitialTransition = true;
	}

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
	if (m_NearTileCoC)
	{
		if (!InitializeNearTile()) return false;
	}

	if (resizeFinal) m_DebugDrawer.Init(m_Final->GetFrameBuffer(), 0, 0, 1, 1);

	return true;
}

bool KDepthOfField::DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	return m_DebugDrawer.Render(renderPass, commandList);
}

bool KDepthOfField::Execute(KRHICommandList& commandList)
{
	const KCamera* camera = KRenderGlobal::Renderer.GetCamera();
	bool validParams =
		std::isfinite(m_FStop) && m_FStop > 0.0f &&
		std::isfinite(m_FocusDistance) && std::isfinite(m_FocalLength) &&
		m_FocalLength > 0.0f && m_FocusDistance > m_FocalLength &&
		m_InputWidth > 0 && m_InputHeight > 0 && m_Width > 0 && m_Height > 0 &&
		m_InputWidth == KRenderGlobal::GBuffer.GetWidth() &&
		m_InputHeight == KRenderGlobal::GBuffer.GetHeight() &&
		camera && camera->GetProjectiveMode() == KCamera::PM_PERSPECTIVE;

	float opticalCoCScale = 0.0f;
	float radiusScaleFull = 0.0f;
	if (validParams)
	{
		const float fovY = camera->GetFov();
		const float aspect = camera->GetAspect();
		const float nearPlane = camera->GetNear();
		const float farPlane = camera->GetFar();
		validParams = std::isfinite(fovY) && fovY > 0.0f && fovY < glm::radians(180.0f) &&
			std::isfinite(aspect) && aspect > 0.0f &&
			std::isfinite(nearPlane) && std::isfinite(farPlane) &&
			nearPlane > 0.0f && farPlane > nearPlane;
		if (validParams)
		{
			// 当前模型只支持相机生成的标准透视；非对称投影不能套用单一 FOV 尺度。
			const glm::mat4& projection = camera->GetProjectiveMatrix();
			glm::mat4 expected = glm::perspectiveRH(fovY, aspect, nearPlane, farPlane);
			expected[1][1] *= -1.0f;
			for (uint32_t column = 0; column < 4; ++column)
			{
				for (uint32_t row = 0; row < 4; ++row)
				{
					validParams = validParams && std::isfinite(projection[column][row]) &&
						std::abs(projection[column][row] - expected[column][row]) <=
						1e-4f * std::max(1.0f, std::abs(expected[column][row]));
				}
			}
			const double pixelScaleX = double(m_InputWidth) * std::abs(projection[0][0]);
			const double pixelScaleY = double(m_InputHeight) * std::abs(projection[1][1]);
			validParams = validParams && std::isfinite(pixelScaleX) && std::isfinite(pixelScaleY) &&
				pixelScaleX > 0.0 && pixelScaleY > 0.0 &&
				std::abs(pixelScaleX - pixelScaleY) <= 1e-3 * std::max(pixelScaleX, pixelScaleY);
		}
		if (validParams)
		{
			// FOV 为弧度制垂直视场角。成像面、焦距和深度都使用场景长度单位。
			const double focalLength = m_FocalLength;
			const double aperture = focalLength / m_FStop;
			const double sensorHeight = 2.0 * focalLength * std::tan(double(fovY) * 0.5);
			opticalCoCScale = float(aperture * focalLength / (double(m_FocusDistance) - focalLength));
			// 0.5 将光学 CoC 直径变成半径，与后续工作分辨率降采样无关。
			radiusScaleFull = float((aperture * focalLength / (double(m_FocusDistance) - focalLength)) *
				0.5 * double(m_InputHeight) / sensorHeight);
			validParams = std::isfinite(opticalCoCScale) && opticalCoCScale > 0.0f &&
				std::isfinite(radiusScaleFull) && radiusScaleFull > 0.0f;
		}
	}

	if (validParams)
	{
		m_InvalidParamsLogged = false;
	}
	else
	{
		opticalCoCScale = radiusScaleFull = 0.0f;
		if (!m_InvalidParamsLogged)
		{
			KG_LOGW(LM_RENDER, "Invalid DOF optics, projection or resolution; disable blur until valid");
			m_InvalidParamsLogged = true;
		}
	}

	struct ObjectData
	{
		// 对焦距离、光学 CoC 直径系数、全分辨率像素半径系数、工作/全分辨率像素尺度。
		glm::vec4 dofParams;
		// 参数有效标志、诊断视图（0 为最终图像）、Tile 生效标志、保留；与 GLSL 保持 16 字节对齐。
		glm::vec4 dofParams2;
	} objectData = {};

	// 使用整数因子的倒数，而非取整后的 workHeight/fullHeight，避免 padding 改变光学尺度。
	objectData.dofParams = glm::vec4(validParams ? m_FocusDistance : 0.0f, opticalCoCScale, radiusScaleFull,
		1.0f / float(m_DownsampleFactor));
	const int debugView = m_DebugView >= DV_FINAL && m_DebugView < DV_COUNT ? m_DebugView : DV_FINAL;
	const bool useTile = m_Enable && m_NearTileEnable && m_NearTileCoC;
	objectData.dofParams2 = glm::vec4(m_Enable && validParams ? 1.0f : 0.0f, float(debugView), useTile ? 1.0f : 0.0f, 0.0f);

	auto ToReadOnly = [&](IKRenderTargetPtr& target)
	{
		// 明确排序附件写入与后续采样；中间纹理的复用依赖这些 GPU 屏障，不需要 CPU 等待。
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
	commandList.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER | PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	auto InitializeReadOnly = [&](IKRenderTargetPtr& target)
	{
		commandList.Transition(target->GetFrameBuffer(), PIPELINE_STAGE_TOP_OF_PIPE, PIPELINE_STAGE_FRAGMENT_SHADER,
			IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_SHADER_READ_ONLY);
	};
	if (m_NeedInitialTransition)
	{
		InitializeReadOnly(m_NearColor);
		InitializeReadOnly(m_FarColor);
		for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
		{
			InitializeReadOnly(intermediate);
		}
		m_NeedInitialTransition = false;
	}
	if (m_FinalNeedsInitialTransition)
	{
		InitializeReadOnly(m_Final);
		m_FinalNeedsInitialTransition = false;
	}
	if (m_NearTileCoC && m_TileNeedsInitialTransition)
	{
		// Composite 的 Storage Image 描述符始终声明 GENERAL，即使当帧关闭 Tile/DOF 也须匹配布局。
		// 因此首次布局转换不能藏在 useTile 分支中；内容仍由首次 MAX 调度完整写入。
		commandList.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_TOP_OF_PIPE,
			PIPELINE_STAGE_COMPUTE_SHADER | PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_GENERAL);
		m_TileNeedsInitialTransition = false;
	}

	if (m_Enable)
	{
	ToColorAttachment(m_NearColor);
	ToColorAttachment(m_FarColor);
	DrawPass("DOF_Preprocess", m_PreprocessPass, m_PreprocessPipeline, true);
	ToReadOnly(m_NearColor);
	ToReadOnly(m_FarColor);

	if (useTile)
	{
		// KApp 的 Storage Image 采样描述符使用 GENERAL；同布局屏障同步 CS 写与 H/V/Composite 采样。
		commandList.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
			PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
		commandList.BeginDebugMarker("DOF_NearTileMAX", glm::vec4(1));
		KDynamicConstantBufferUsage tileUsage;
		tileUsage.binding = SHADER_BINDING_OBJECT;
		tileUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, tileUsage);
		commandList.Execute(m_NearTileMaxPipeline, (m_Width + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE,
			(m_Height + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE, 1, &tileUsage);
		commandList.EndDebugMarker();
		commandList.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
			PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
	}

	ToColorAttachment(m_ComplexIntermediate[0]);
	ToColorAttachment(m_ComplexIntermediate[1]);
	DrawPass("DOF_NearHorizontal", m_NearHorizontalPass, useTile ? m_NearTileHorizontalPipeline : m_NearHorizontalPipeline, false);
	ToReadOnly(m_ComplexIntermediate[0]);
	ToReadOnly(m_ComplexIntermediate[1]);

	ToColorAttachment(m_NearColor);
	DrawPass("DOF_NearVertical", m_NearVerticalPass, useTile ? m_NearTileVerticalPipeline : m_NearVerticalPipeline, false);
	ToReadOnly(m_NearColor);

	// Near V 已读完前两张复数纹理，此后 Far H 可覆盖全部四张；跨帧仍回到只读布局。
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
	}

	ToColorAttachment(m_Final);
	DrawPass("DOF_Composite", m_CompositePass, m_CompositePipeline, true);
	ToReadOnly(m_Final);

	commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	commandList.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER | PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	commandList.EndDebugMarker();

	return true;
}
