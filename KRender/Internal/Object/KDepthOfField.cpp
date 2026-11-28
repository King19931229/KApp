#include "KDepthOfField.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"
#include "Product/Shaders/dof/tile_config.h"
#include <algorithm>
#include <cmath>
#include <limits>

KDepthOfField::KDepthOfField()
	: m_FStop(1.0f)
	, m_FocusDistance(200.0f)
	, m_FocalLength(10.0f)
	, m_DebugView(DV_FINAL)
	, m_Enable(true)
	, m_NearTileEnable(true)
	, m_BracketedEnable(true)
	, m_UseComputePreprocess(true)
	, m_FocusMode(KDOFFocusMode::AutoFocusGPU)
	, m_PrevFocusMode(KDOFFocusMode::Manual)
	, m_FrameDeltaTime(0.0f)
	, m_AutoFocusReady(false)
	, m_AutoFocusActive(false)
	, m_AutoFocusUseCounter(0)
	, m_CameraCutGeneration(0)
	, m_ManualResetGeneration(0)
	, m_NearTileSupported(false)
	, m_ComputePreprocessSupported(false)
	, m_BracketedSupported(false)
	, m_NearTileReady(false)
	, m_NearTileActive(false)
	, m_ComputePreprocessReady(false)
	, m_ComputePreprocessTileReady(false)
	, m_ComputePreprocessActive(false)
	, m_BracketedActive(false)
	, m_LastTileAttemptFailed(false)
	, m_LastComputeAttemptFailed(false)
	, m_LastComputeTileAttemptFailed(false)
	, m_LastBracketedAttemptFailed(false)
	, m_WorkingResolution(0)
	, m_DownsampleFactor(2)
	, m_InputWidth(0)
	, m_InputHeight(0)
	, m_Width(0)
	, m_Height(0)
	, m_NeedInitialTransition(true)
	, m_ColorMeanNeedsInitialTransition(false)
	, m_FinalNeedsInitialTransition(true)
	, m_TileNeedsInitialTransition(true)
	, m_InvalidParamsLogged(false)
{
}

KDepthOfField::~KDepthOfField()
{
}

void KDepthOfField::QueryCapabilities()
{
	KRenderDeviceProperties* properties = nullptr;
	const bool queried = KRenderGlobal::RenderDevice->QueryProperty(&properties) && properties != nullptr;
	m_NearTileSupported = queried && properties->r16fStorageImageSupport;
	m_ComputePreprocessSupported = queried && properties->rgba16fStorageImageSupport;
	const bool fiveMrtSupported = queried &&
		properties->maxColorAttachments >= 5 &&
		properties->maxFragmentOutputAttachments >= 5;
	m_BracketedSupported = fiveMrtSupported && properties->rgba8UnormColorAttachmentSupport;
}

bool KDepthOfField::IsSceneColorNonNegativeGuaranteed(std::string& reason) const
{
	const DeferredRenderDebug debugOption = KRenderGlobal::DeferredRenderer.GetDebugOption();
	if (debugOption == DRD_NORMAL || debugOption == DRD_MOTION)
	{
		reason = "Signed SceneColor debug output (Normal/Motion); fallback to baseline FP16";
		return false;
	}
	return true;
}

bool KDepthOfField::CanUseBracketed(std::string& reason) const
{
	if (!m_BracketedSupported)
	{
		reason = "Bracketing unavailable (requires 5 MRT and RGBA8_UNORM); fallback to baseline FP16";
		return false;
	}
	if (!IsSceneColorNonNegativeGuaranteed(reason))
	{
		if (reason.empty())
		{
			reason = "SceneColor non-negative contract not satisfied; fallback to baseline FP16";
		}
		return false;
	}
	reason.clear();
	return true;
}

bool KDepthOfField::EnsureBlurShaders(bool bracketed, bool nearTile)
{
	const bool async = KRenderGlobal::EnableAsyncShaderCompile;
	auto AcquireIfNeeded = [async](const KShaderCompileEnvironment& env, const char* file, KShaderRef& shader)
	{
		if (!shader)
		{
			KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, file, env, shader, async);
		}
	};

	KShaderCompileEnvironment env;
	if (bracketed)
	{
		env.macros.push_back({ "DOF_BRACKETED", "1" });
	}
	if (nearTile)
	{
		env.macros.push_back({ "DOF_NEAR_TILE", "1" });
		KShaderRef& hShader = bracketed ? m_NearTileBracketedHorizontalFS : m_NearTileHorizontalFS;
		KShaderRef& vShader = bracketed ? m_NearTileBracketedVerticalFS : m_NearTileVerticalFS;
		AcquireIfNeeded(env, "dof/horizontal.frag", hShader);
		AcquireIfNeeded(env, "dof/vertical.frag", vShader);
		return bool(hShader) && bool(vShader);
	}

	KShaderRef& nearH = bracketed ? m_NearBracketedHorizontalFS : m_NearHorizontalFS;
	KShaderRef& nearV = bracketed ? m_NearBracketedVerticalFS : m_NearVerticalFS;
	AcquireIfNeeded(env, "dof/horizontal.frag", nearH);
	AcquireIfNeeded(env, "dof/vertical.frag", nearV);

	KShaderCompileEnvironment farEnv = env;
	farEnv.macros.push_back({ "DOF_FAR", "1" });
	KShaderRef& farH = bracketed ? m_FarBracketedHorizontalFS : m_FarHorizontalFS;
	KShaderRef& farV = bracketed ? m_FarBracketedVerticalFS : m_FarVerticalFS;
	AcquireIfNeeded(farEnv, "dof/horizontal.frag", farH);
	AcquireIfNeeded(farEnv, "dof/vertical.frag", farV);

	return bool(nearH) && bool(nearV) && bool(farH) && bool(farV);
}

bool KDepthOfField::EnsureAutoFocusResources()
{
	struct AutoFocusViewGPUData
	{
		glm::vec4 dofParams;
		float focusU;
		uint32_t focusState;
		uint32_t initialized;
		uint32_t lastValidCount;
	};
	static_assert(sizeof(AutoFocusViewGPUData) == 32, "AutoFocusViewGPUData must match GLSL std430 layout");

	if (!m_AutoFocusBuffer)
	{
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_AutoFocusBuffer);
		if (!m_AutoFocusBuffer)
		{
			m_AutoFocusFallbackReason = "AutoFocus SSBO allocation failed";
			return false;
		}
		std::vector<AutoFocusViewGPUData> zeroInit(DOF_AUTOFOCUS_MAX_VIEWS);
		memset(zeroInit.data(), 0, zeroInit.size() * sizeof(AutoFocusViewGPUData));
		if (!m_AutoFocusBuffer->InitMemory(zeroInit.size() * sizeof(AutoFocusViewGPUData), zeroInit.data()) ||
			!m_AutoFocusBuffer->InitDevice(false, false, true))
		{
			SAFE_UNINIT(m_AutoFocusBuffer);
			m_AutoFocusFallbackReason = "AutoFocus SSBO device init failed";
			return false;
		}
		m_AutoFocusBuffer->SetDebugName("DOF_AutoFocusBuffer");
	}

	if (!m_AutoFocusPipeline)
	{
		KRenderGlobal::RenderDevice->CreateComputePipeline(m_AutoFocusPipeline);
		if (!m_AutoFocusPipeline)
		{
			m_AutoFocusFallbackReason = "AutoFocus compute pipeline allocation failed";
			return false;
		}
		m_AutoFocusPipeline->BindSampler(
			SHADER_BINDING_TEXTURE0,
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
		m_AutoFocusPipeline->BindStorageBuffer(
			SHADER_BINDING_TEXTURE5,
			m_AutoFocusBuffer,
			COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT,
			false);
		m_AutoFocusPipeline->BindUniformBuffer(
			CBT_CAMERA,
			KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA));
		m_AutoFocusPipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
		if (!m_AutoFocusPipeline->Init("dof/autofocus.comp", KShaderCompileEnvironment(), false))
		{
			SAFE_UNINIT(m_AutoFocusPipeline);
			m_AutoFocusFallbackReason = "AutoFocus compute shader/pipeline init failed";
			return false;
		}
		m_AutoFocusPipeline->SetDebugName("DOF_AutoFocusPipeline");
	}

	m_AutoFocusReady = true;
	m_AutoFocusFallbackReason.clear();
	return true;
}

uint32_t KDepthOfField::AcquireAutoFocusViewSlot(const KCamera* camera, uint32_t& outResetMode)
{
	uint32_t slotIndex = DOF_AUTOFOCUS_MAX_VIEWS;
	uint32_t lruIndex = 0;
	uint64_t lruStamp = std::numeric_limits<uint64_t>::max();
	for (uint32_t i = 0; i < DOF_AUTOFOCUS_MAX_VIEWS; ++i)
	{
		if (m_AutoFocusViews[i].camera == camera && m_AutoFocusViews[i].initialized)
		{
			slotIndex = i;
			break;
		}
		if (!m_AutoFocusViews[i].initialized && slotIndex == DOF_AUTOFOCUS_MAX_VIEWS)
		{
			slotIndex = i;
		}
		if (m_AutoFocusViews[i].lastUsedStamp < lruStamp)
		{
			lruStamp = m_AutoFocusViews[i].lastUsedStamp;
			lruIndex = i;
		}
	}

	bool newlyAssigned = false;
	if (slotIndex == DOF_AUTOFOCUS_MAX_VIEWS)
	{
		slotIndex = lruIndex;
		newlyAssigned = true;
	}
	else if (!m_AutoFocusViews[slotIndex].initialized || m_AutoFocusViews[slotIndex].camera != camera)
	{
		newlyAssigned = true;
	}

	AutoFocusViewSlot& slot = m_AutoFocusViews[slotIndex];
	if (newlyAssigned)
	{
		slot.camera = camera;
		slot.initialized = false;
		slot.pendingManualReset = true;
		slot.pendingCameraCut = false;
	}

	if (slot.pendingCameraCut || slot.cameraCutGeneration != m_CameraCutGeneration)
	{
		outResetMode = DOF_AUTOFOCUS_RESET_CAMERA_CUT;
	}
	else if (!slot.initialized || slot.pendingManualReset || slot.manualResetGeneration != m_ManualResetGeneration)
	{
		outResetMode = DOF_AUTOFOCUS_RESET_MANUAL;
	}
	else
	{
		outResetMode = DOF_AUTOFOCUS_RESET_NONE;
	}

	slot.initialized = true;
	slot.pendingCameraCut = false;
	slot.pendingManualReset = false;
	slot.cameraCutGeneration = m_CameraCutGeneration;
	slot.manualResetGeneration = m_ManualResetGeneration;
	slot.lastUsedStamp = ++m_AutoFocusUseCounter;
	return slotIndex;
}

void KDepthOfField::SetFocusMode(KDOFFocusMode mode)
{
	if (m_FocusMode != mode)
	{
		m_FocusMode = mode;
		if (m_FocusMode == KDOFFocusMode::AutoFocusGPU)
		{
			++m_ManualResetGeneration;
		}
	}
}

void KDepthOfField::ResetAutoFocus(bool cameraCut)
{
	if (cameraCut)
	{
		++m_CameraCutGeneration;
	}
	else
	{
		++m_ManualResetGeneration;
		for (uint32_t i = 0; i < DOF_AUTOFOCUS_MAX_VIEWS; ++i)
		{
			m_AutoFocusViews[i].pendingCameraCut = false;
		}
	}
}

void KDepthOfField::NotifyCameraCut(const KCamera* camera)
{
	if (!camera)
	{
		++m_CameraCutGeneration;
		return;
	}
	uint32_t emptyIndex = DOF_AUTOFOCUS_MAX_VIEWS;
	uint32_t lruIndex = 0;
	uint64_t lruStamp = std::numeric_limits<uint64_t>::max();
	for (uint32_t i = 0; i < DOF_AUTOFOCUS_MAX_VIEWS; ++i)
	{
		if (m_AutoFocusViews[i].camera == camera && m_AutoFocusViews[i].initialized)
		{
			m_AutoFocusViews[i].pendingCameraCut = true;
			return;
		}
		if (!m_AutoFocusViews[i].initialized && emptyIndex == DOF_AUTOFOCUS_MAX_VIEWS)
		{
			emptyIndex = i;
		}
		if (m_AutoFocusViews[i].lastUsedStamp < lruStamp)
		{
			lruStamp = m_AutoFocusViews[i].lastUsedStamp;
			lruIndex = i;
		}
	}
	// 相机尚未分配槽位时，仅为其预占槽位并标记待吸附，避免误触发其他视图的全局 CameraCut。
	const uint32_t slotIndex = (emptyIndex != DOF_AUTOFOCUS_MAX_VIEWS) ? emptyIndex : lruIndex;
	AutoFocusViewSlot& slot = m_AutoFocusViews[slotIndex];
	slot.camera = camera;
	slot.initialized = true;
	slot.pendingCameraCut = true;
	slot.pendingManualReset = false;
	slot.cameraCutGeneration = m_CameraCutGeneration;
	slot.manualResetGeneration = m_ManualResetGeneration;
	slot.lastUsedStamp = ++m_AutoFocusUseCounter;
}

bool KDepthOfField::ConfigureBlurPipeline(IKPipelinePtr& pipeline, const KShaderRef& fragment,
	IKFrameBufferPtr tex0, IKFrameBufferPtr tex1, IKFrameBufferPtr tex2, IKFrameBufferPtr tex3,
	IKFrameBufferPtr colorMean, IKRenderPassPtr renderPass, bool dynamicWrite,
	IKStorageBufferPtr autoFocusBuffer)
{
	if (!m_QuadVS || !fragment || !renderPass)
	{
		return false;
	}
	if (!pipeline)
	{
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);
	}
	pipeline->UnInit(true);

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

	IKSamplerPtr sampler = KRenderGlobal::GBuffer.GetSampler();
	IKFrameBufferPtr textures[5] = { tex0, tex1, tex2, tex3, colorMean };
	uint32_t bindings[5] = { SHADER_BINDING_TEXTURE0, SHADER_BINDING_TEXTURE1, SHADER_BINDING_TEXTURE2, SHADER_BINDING_TEXTURE3, SHADER_BINDING_TEXTURE4 };
	for (uint32_t i = 0; i < 5; ++i)
	{
		if (textures[i])
		{
			pipeline->SetSampler(bindings[i], textures[i], sampler, dynamicWrite);
		}
	}
	if (autoFocusBuffer)
	{
		pipeline->SetStorageBuffer(SHADER_BINDING_TEXTURE5, ST_FRAGMENT, autoFocusBuffer);
	}
	pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA));

	if (!pipeline->Init())
	{
		return false;
	}
	IKPipelineHandlePtr handle;
	// 提前校验着色器编译、Descriptor Layout 与多 MRT Graphics Pipeline 创建，避免延迟到 Execute 失败。
	return pipeline->GetHandle(renderPass, handle) && bool(handle);
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

	m_NearTileReady = false;
	m_NearTileActive = false;
	m_ComputePreprocessReady = false;
	m_ComputePreprocessTileReady = false;
	m_ComputePreprocessActive = false;
	m_BracketedActive = false;
	m_LastTileAttemptFailed = false;
	m_LastComputeAttemptFailed = false;
	m_LastComputeTileAttemptFailed = false;
	m_LastBracketedAttemptFailed = false;
	m_NearTileFallbackReason.clear();
	m_ComputePreprocessFallbackReason.clear();
	m_BracketedFallbackReason.clear();
	m_AutoFocusReady = false;
	m_AutoFocusActive = false;
	m_AutoFocusFallbackReason.clear();
	m_PrevFocusMode = KDOFFocusMode::Manual;
	m_AutoFocusUseCounter = 0;
	m_CameraCutGeneration = 0;
	m_ManualResetGeneration = 0;
	for (uint32_t i = 0; i < DOF_AUTOFOCUS_MAX_VIEWS; ++i)
	{
		m_AutoFocusViews[i] = AutoFocusViewSlot();
	}

	QueryCapabilities();
	if (!EnsureAutoFocusResources())
	{
		return false;
	}

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/preprocess.frag", m_PreprocessFS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "dof/composite.frag", m_CompositeFS, KRenderGlobal::EnableAsyncShaderCompile);
	if (!EnsureBlurShaders(false, false))
	{
		return false;
	}
	if (!m_Final)
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_Final);
	}

	if (m_BracketedEnable && CanUseBracketed(m_BracketedFallbackReason))
	{
		m_BracketedActive = true;
		if (Resize(width, height))
		{
			return true;
		}
		m_BracketedActive = false;
		m_LastBracketedAttemptFailed = true;
		m_BracketedFallbackReason = "Bracketing 5-MRT initialization failed; fallback to baseline FP16";
		KG_LOGW(LM_RENDER, "%s", m_BracketedFallbackReason.c_str());
	}

	return Resize(width, height);
}

bool KDepthOfField::UnInit()
{
	// 与既有 UnInit 契约相同，调用方已进入安全销毁边界。
	ReleaseRetiredWorkingResources(true);
	m_LastFrameFence.reset();
	m_DebugDrawer.UnInit();

	SAFE_UNINIT(m_AutoFocusPipeline);
	SAFE_UNINIT(m_AutoFocusBuffer);
	SAFE_UNINIT(m_NearTileMaxPipeline);
	SAFE_UNINIT(m_PreprocessComputePipeline);
	SAFE_UNINIT(m_PreprocessComputeTilePipeline);
	SAFE_UNINIT(m_NearTileHorizontalPipeline);
	SAFE_UNINIT(m_NearTileVerticalPipeline);
	SAFE_UNINIT(m_NearTileCoC);

	m_NearTileHorizontalFS.Release();
	m_NearTileVerticalFS.Release();
	m_NearTileBracketedHorizontalFS.Release();
	m_NearTileBracketedVerticalFS.Release();

	SAFE_UNINIT(m_Final);
	SAFE_UNINIT(m_HorizontalColorMean);
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
	m_NearBracketedHorizontalFS.Release();
	m_NearBracketedVerticalFS.Release();
	m_FarBracketedHorizontalFS.Release();
	m_FarBracketedVerticalFS.Release();
	m_CompositeFS.Release();

	m_NearTileReady = false;
	m_NearTileActive = false;
	m_ComputePreprocessReady = false;
	m_ComputePreprocessTileReady = false;
	m_ComputePreprocessActive = false;
	m_BracketedActive = false;
	m_AutoFocusReady = false;
	m_AutoFocusActive = false;
	m_PrevFocusMode = KDOFFocusMode::Manual;
	for (uint32_t i = 0; i < DOF_AUTOFOCUS_MAX_VIEWS; ++i)
	{
		m_AutoFocusViews[i] = AutoFocusViewSlot();
	}
	m_InputWidth = 0;
	m_InputHeight = 0;
	m_Width = 0;
	m_Height = 0;

	return true;
}

bool KDepthOfField::HasPendingResolutionChange() const
{
	return m_DownsampleFactor != (m_WorkingResolution == 1 ? 4U : 2U);
}

void KDepthOfField::RetiredWorkingResources::UnInit()
{
	for (auto& computePipeline : computePipelines)
	{
		SAFE_UNINIT(computePipeline);
	}
	computePipelines.clear();
	for (auto& pipeline : pipelines)
	{
		// 本组独占 RenderPass 的最后使用 Fence 已完成，销毁 Handle 不应再等待整个设备。
		if (pipeline)
		{
			pipeline->UnInit(true);
		}
		pipeline.reset();
	}
	pipelines.clear();
	for (auto& pass : passes)
	{
		SAFE_UNINIT(pass);
	}
	passes.clear();
	for (auto& target : targets)
	{
		SAFE_UNINIT(target);
	}
	targets.clear();
	for (auto& shader : shaders)
	{
		shader.Release();
	}
	shaders.clear();
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
		else
		{
			++it;
		}
	}
}

bool KDepthOfField::RebuildWorkingResources(uint32_t width, uint32_t height, uint32_t downsampleFactor,
	bool bracketed, bool retireCurrent, PixelFormat bracketedFormat)
{
	if (!width || !height || !downsampleFactor)
	{
		return false;
	}
	if (!EnsureBlurShaders(bracketed, false))
	{
		return false;
	}

	const uint32_t workWidth = width / downsampleFactor + (width % downsampleFactor != 0);
	const uint32_t workHeight = height / downsampleFactor + (height % downsampleFactor != 0);
	const bool resizeFinal = !m_Final || !m_Final->GetFrameBuffer() || !m_Final->GetFrameBuffer()->GetWidth() ||
		width != m_InputWidth || height != m_InputHeight;
	const PixelFormat intermediateFormat = bracketed ? bracketedFormat : PF_R16G16B16A16_FLOAT;

	// 事务式在临时对象中完整构建；任一步失败即原地清理临时对象，不破坏当前活动路径。
	RetiredWorkingResources staged;
	auto CleanupStaged = [&]()
	{
		staged.UnInit();
	};

	IKRenderTargetPtr newNearColor;
	IKRenderTargetPtr newFarColor;
	IKRenderTargetPtr newIntermediate[4];
	IKRenderTargetPtr newColorMean;

	auto CreateColorTarget = [&](IKRenderTargetPtr& target, uint32_t w, uint32_t h, PixelFormat format) -> bool
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(target);
		if (!target)
		{
			return false;
		}
		staged.targets.push_back(target);
		// 初始布局转换放入正常帧命令，避开既有单次提交辅助函数中的 vkWaitForFences。
		return target->InitFromColor(w, h, 1, 1, format, true);
	};

	if (!CreateColorTarget(newNearColor, workWidth, workHeight, PF_R16G16B16A16_FLOAT) ||
		!CreateColorTarget(newFarColor, workWidth, workHeight, PF_R16G16B16A16_FLOAT))
	{
		CleanupStaged();
		return false;
	}
	for (uint32_t i = 0; i < ARRAY_SIZE(newIntermediate); ++i)
	{
		if (!CreateColorTarget(newIntermediate[i], workWidth, workHeight, intermediateFormat))
		{
			CleanupStaged();
			return false;
		}
	}
	if (bracketed)
	{
		if (!CreateColorTarget(newColorMean, workWidth, workHeight, PF_R16G16B16A16_FLOAT))
		{
			CleanupStaged();
			return false;
		}
		newColorMean->GetFrameBuffer()->SetDebugName("DOF_HorizontalColorMean");
	}
	if (resizeFinal)
	{
		// KDeferredRenderer 的 m_DrawProcessResultPipeline 长期持有 m_Final->GetFrameBuffer()（dynamicWrite=true），
		// 且 OnSwapChainRecreate 先于 DOF 调整 DeferredRenderer；必须原地重建 m_Final，保持 IKFrameBuffer 指针稳定。
		if (retireCurrent)
		{
			CleanupStaged();
			return false;
		}
		m_DebugDrawer.UnInit();
		if (!m_Final)
		{
			KRenderGlobal::RenderDevice->CreateRenderTarget(m_Final);
		}
		if (!m_Final || !m_Final->InitFromColor(width, height, 1, 1, PF_R16G16B16A16_FLOAT, true))
		{
			CleanupStaged();
			return false;
		}
		m_Final->GetFrameBuffer()->SetDebugName("DOF_Final");
	}

	IKRenderPassPtr newPreprocessPass;
	IKRenderPassPtr newNearHorizontalPass;
	IKRenderPassPtr newNearVerticalPass;
	IKRenderPassPtr newFarHorizontalPass;
	IKRenderPassPtr newFarVerticalPass;
	IKRenderPassPtr newCompositePass;

	auto CreatePass = [&](IKRenderPassPtr& pass) -> bool
	{
		KRenderGlobal::RenderDevice->CreateRenderPass(pass);
		if (!pass)
		{
			return false;
		}
		staged.passes.push_back(pass);
		return true;
	};

	if (!CreatePass(newPreprocessPass) || !CreatePass(newNearHorizontalPass) ||
		!CreatePass(newNearVerticalPass) || !CreatePass(newFarHorizontalPass) ||
		!CreatePass(newFarVerticalPass) || !CreatePass(newCompositePass))
	{
		CleanupStaged();
		return false;
	}

	auto SetColorSlot = [](IKRenderPassPtr& pass, uint32_t slot, IKFrameBufferPtr fb) -> bool
	{
		return pass->SetColorAttachment(slot, fb) &&
			pass->SetOpColor(slot, LO_DONT_CARE, SO_STORE) &&
			pass->SetClearColor(slot, { 0.0f, 0.0f, 0.0f, 0.0f });
	};

	bool passesOk =
		SetColorSlot(newPreprocessPass, 0, newNearColor->GetFrameBuffer()) &&
		SetColorSlot(newPreprocessPass, 1, newFarColor->GetFrameBuffer()) &&
		newPreprocessPass->Init() &&
		SetColorSlot(newNearHorizontalPass, 0, newIntermediate[0]->GetFrameBuffer()) &&
		SetColorSlot(newNearHorizontalPass, 1, newIntermediate[1]->GetFrameBuffer()) &&
		(!bracketed || SetColorSlot(newNearHorizontalPass, 2, newColorMean->GetFrameBuffer())) &&
		newNearHorizontalPass->Init() &&
		SetColorSlot(newNearVerticalPass, 0, newNearColor->GetFrameBuffer()) &&
		newNearVerticalPass->Init();
	if (passesOk)
	{
		for (uint32_t i = 0; i < ARRAY_SIZE(newIntermediate); ++i)
		{
			passesOk = passesOk && SetColorSlot(newFarHorizontalPass, i, newIntermediate[i]->GetFrameBuffer());
		}
		if (bracketed)
		{
			passesOk = passesOk && SetColorSlot(newFarHorizontalPass, 4, newColorMean->GetFrameBuffer());
		}
		passesOk = passesOk && newFarHorizontalPass->Init() &&
			SetColorSlot(newFarVerticalPass, 0, newFarColor->GetFrameBuffer()) &&
			newFarVerticalPass->Init() &&
			SetColorSlot(newCompositePass, 0, m_Final->GetFrameBuffer()) &&
			newCompositePass->Init();
	}
	if (!passesOk)
	{
		CleanupStaged();
		return false;
	}

	IKPipelinePtr newPreprocessPipeline;
	IKPipelinePtr newNearHorizontalPipeline;
	IKPipelinePtr newNearVerticalPipeline;
	IKPipelinePtr newFarHorizontalPipeline;
	IKPipelinePtr newFarVerticalPipeline;
	IKPipelinePtr newCompositePipeline;

	auto StagePipeline = [&](IKPipelinePtr& pipeline) -> bool
	{
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);
		if (!pipeline)
		{
			return false;
		}
		staged.pipelines.push_back(pipeline);
		return true;
	};

	if (!StagePipeline(newPreprocessPipeline) || !StagePipeline(newNearHorizontalPipeline) ||
		!StagePipeline(newNearVerticalPipeline) || !StagePipeline(newFarHorizontalPipeline) ||
		!StagePipeline(newFarVerticalPipeline) || !StagePipeline(newCompositePipeline))
	{
		CleanupStaged();
		return false;
	}

	const KShaderRef& nearHFS = bracketed ? m_NearBracketedHorizontalFS : m_NearHorizontalFS;
	const KShaderRef& nearVFS = bracketed ? m_NearBracketedVerticalFS : m_NearVerticalFS;
	const KShaderRef& farHFS = bracketed ? m_FarBracketedHorizontalFS : m_FarHorizontalFS;
	const KShaderRef& farVFS = bracketed ? m_FarBracketedVerticalFS : m_FarVerticalFS;
	IKFrameBufferPtr meanFB = bracketed ? newColorMean->GetFrameBuffer() : nullptr;

	bool pipelinesOk =
		ConfigureBlurPipeline(newPreprocessPipeline, m_PreprocessFS,
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(),
			nullptr, nullptr, nullptr, newPreprocessPass, false, m_AutoFocusBuffer) &&
		ConfigureBlurPipeline(newNearHorizontalPipeline, nearHFS,
			newNearColor->GetFrameBuffer(), nullptr, nullptr, nullptr, nullptr,
			newNearHorizontalPass, false) &&
		ConfigureBlurPipeline(newNearVerticalPipeline, nearVFS,
			newIntermediate[0]->GetFrameBuffer(), newIntermediate[1]->GetFrameBuffer(),
			nullptr, nullptr, meanFB, newNearVerticalPass, false) &&
		ConfigureBlurPipeline(newFarHorizontalPipeline, farHFS,
			newFarColor->GetFrameBuffer(), nullptr, nullptr, nullptr, nullptr,
			newFarHorizontalPass, false) &&
		ConfigureBlurPipeline(newFarVerticalPipeline, farVFS,
			newIntermediate[0]->GetFrameBuffer(), newIntermediate[1]->GetFrameBuffer(),
			newIntermediate[2]->GetFrameBuffer(), newIntermediate[3]->GetFrameBuffer(),
			meanFB, newFarVerticalPass, false) &&
		ConfigureBlurPipeline(newCompositePipeline, m_CompositeFS,
			KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
			newNearColor->GetFrameBuffer(), newFarColor->GetFrameBuffer(),
			newNearColor->GetFrameBuffer(), newCompositePass, true, m_AutoFocusBuffer);

	if (!pipelinesOk)
	{
		CleanupStaged();
		return false;
	}

	// 新工作资源全部校验就绪，现在提交切换并安全退役/释放旧资源。
	staged.targets.clear();
	staged.passes.clear();
	staged.pipelines.clear();

	if (retireCurrent && m_NearColor)
	{
		RetiredWorkingResources retired;
		retired.completionFence = m_LastFrameFence;
		// 旧描述符、Framebuffer、Image 和 Compute Pipeline 一起保留到最后使用 Fence 完成。
		for (IKRenderTargetPtr* target : { &m_NearColor, &m_FarColor, &m_ComplexIntermediate[0],
			&m_ComplexIntermediate[1], &m_ComplexIntermediate[2], &m_ComplexIntermediate[3], &m_HorizontalColorMean,
			&m_NearTileCoC })
		{
			if (*target)
			{
				retired.targets.push_back(std::move(*target));
			}
		}
		for (IKRenderPassPtr* pass : { &m_PreprocessPass, &m_NearHorizontalPass, &m_NearVerticalPass,
			&m_FarHorizontalPass, &m_FarVerticalPass, &m_CompositePass })
		{
			if (*pass)
			{
				retired.passes.push_back(std::move(*pass));
			}
		}
		for (IKPipelinePtr* pipeline : { &m_PreprocessPipeline, &m_NearHorizontalPipeline, &m_NearVerticalPipeline,
			&m_FarHorizontalPipeline, &m_FarVerticalPipeline, &m_CompositePipeline,
			&m_NearTileHorizontalPipeline, &m_NearTileVerticalPipeline })
		{
			if (*pipeline)
			{
				retired.pipelines.push_back(std::move(*pipeline));
			}
		}
		for (IKComputePipelinePtr* compute : { &m_NearTileMaxPipeline, &m_PreprocessComputePipeline, &m_PreprocessComputeTilePipeline })
		{
			if (*compute)
			{
				retired.computePipelines.push_back(std::move(*compute));
			}
		}

		m_RetiredWorkingResources.push_back(std::move(retired));
		ReleaseRetiredWorkingResources();
	}
	else
	{
		SAFE_UNINIT(m_NearTileMaxPipeline);
		SAFE_UNINIT(m_PreprocessComputePipeline);
		SAFE_UNINIT(m_PreprocessComputeTilePipeline);
		SAFE_UNINIT(m_NearTileHorizontalPipeline);
		SAFE_UNINIT(m_NearTileVerticalPipeline);
		SAFE_UNINIT(m_NearTileCoC);
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
		SAFE_UNINIT(m_HorizontalColorMean);
		SAFE_UNINIT(m_NearColor);
		SAFE_UNINIT(m_FarColor);
		for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
		{
			SAFE_UNINIT(intermediate);
		}
	}

	m_NearTileReady = false;
	m_ComputePreprocessReady = false;
	m_ComputePreprocessTileReady = false;

	m_DownsampleFactor = downsampleFactor;
	m_InputWidth = width;
	m_InputHeight = height;
	m_Width = workWidth;
	m_Height = workHeight;
	m_BracketedActive = bracketed;

	m_NearColor = std::move(newNearColor);
	m_FarColor = std::move(newFarColor);
	for (uint32_t i = 0; i < ARRAY_SIZE(m_ComplexIntermediate); ++i)
	{
		m_ComplexIntermediate[i] = std::move(newIntermediate[i]);
	}
	m_HorizontalColorMean = std::move(newColorMean);
	if (resizeFinal)
	{
		m_FinalNeedsInitialTransition = true;
		m_DebugDrawer.Init(m_Final->GetFrameBuffer(), 0, 0, 1, 1);
	}

	m_PreprocessPass = std::move(newPreprocessPass);
	m_NearHorizontalPass = std::move(newNearHorizontalPass);
	m_NearVerticalPass = std::move(newNearVerticalPass);
	m_FarHorizontalPass = std::move(newFarHorizontalPass);
	m_FarVerticalPass = std::move(newFarVerticalPass);
	m_CompositePass = std::move(newCompositePass);

	m_PreprocessPipeline = std::move(newPreprocessPipeline);
	m_NearHorizontalPipeline = std::move(newNearHorizontalPipeline);
	m_NearVerticalPipeline = std::move(newNearVerticalPipeline);
	m_FarHorizontalPipeline = std::move(newFarHorizontalPipeline);
	m_FarVerticalPipeline = std::move(newFarVerticalPipeline);
	m_CompositePipeline = std::move(newCompositePipeline);

	m_NeedInitialTransition = true;
	m_ColorMeanNeedsInitialTransition = bool(m_HorizontalColorMean);
	return true;
}

bool KDepthOfField::InitializeNearTile()
{
	if (!m_NearTileSupported)
	{
		m_NearTileFallbackReason = "R16F storage/sampled image unsupported";
		return false;
	}
	if (!m_NearColor || !m_NearHorizontalPass || !m_NearVerticalPass || !m_CompositePipeline)
	{
		m_NearTileFallbackReason = "Working DOF resources not initialized";
		return false;
	}
	if (!EnsureBlurShaders(m_BracketedActive, true))
	{
		m_NearTileFallbackReason = "Near Tile shader acquisition failed";
		return false;
	}

	// 事务式构建：全部成功后再提交成员变量，避免失败后残留非空 m_NearTileCoC 或半初始化管线。
	IKRenderTargetPtr newTileCoC;
	IKComputePipelinePtr newTileMaxPipeline;
	IKPipelinePtr newTileHPipeline;
	IKPipelinePtr newTileVPipeline;
	auto Cleanup = [&]()
	{
		SAFE_UNINIT(newTileMaxPipeline);
		if (newTileHPipeline)
		{
			newTileHPipeline->UnInit(true);
		}
		if (newTileVPipeline)
		{
			newTileVPipeline->UnInit(true);
		}
		SAFE_UNINIT(newTileCoC);
	};

	KRenderGlobal::RenderDevice->CreateRenderTarget(newTileCoC);
	// 共享 Tile 尺寸由同一头文件提供给 C++ 和 GLSL，避免 CPU 分配与 Compute 分组分叉。
	const uint32_t tileWidth = (m_Width + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
	const uint32_t tileHeight = (m_Height + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
	if (!newTileCoC || !newTileCoC->InitFromStorage(tileWidth, tileHeight, 1, PF_R16_FLOAT, true))
	{
		Cleanup();
		m_NearTileFallbackReason = "R16F TileMAX target allocation failed";
		return false;
	}
	newTileCoC->GetFrameBuffer()->SetDebugName("DOF_NearTileCoC");

	KRenderGlobal::RenderDevice->CreateComputePipeline(newTileMaxPipeline);
	if (!newTileMaxPipeline)
	{
		Cleanup();
		m_NearTileFallbackReason = "TileMAX compute pipeline creation failed";
		return false;
	}
	newTileMaxPipeline->BindSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
	newTileMaxPipeline->BindStorageImage(SHADER_BINDING_TEXTURE1, newTileCoC->GetFrameBuffer(), PF_R16_FLOAT, COMPUTE_RESOURCE_OUT, 0, true);
	newTileMaxPipeline->BindStorageBuffer(SHADER_BINDING_TEXTURE5, m_AutoFocusBuffer, COMPUTE_RESOURCE_IN, false);
	newTileMaxPipeline->BindUniformBuffer(CBT_CAMERA, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA));
	newTileMaxPipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	if (!newTileMaxPipeline->Init("dof/near_tile_max.comp", KShaderCompileEnvironment(), false))
	{
		Cleanup();
		m_NearTileFallbackReason = "TileMAX compute shader/pipeline init failed";
		return false;
	}

	const KShaderRef& tileHFS = m_BracketedActive ? m_NearTileBracketedHorizontalFS : m_NearTileHorizontalFS;
	const KShaderRef& tileVFS = m_BracketedActive ? m_NearTileBracketedVerticalFS : m_NearTileVerticalFS;
	IKFrameBufferPtr meanFB = m_BracketedActive && m_HorizontalColorMean ? m_HorizontalColorMean->GetFrameBuffer() : nullptr;

	if (!ConfigureBlurPipeline(newTileHPipeline, tileHFS,
			m_NearColor->GetFrameBuffer(), nullptr, newTileCoC->GetFrameBuffer(), nullptr,
			nullptr, m_NearHorizontalPass, true) ||
		!ConfigureBlurPipeline(newTileVPipeline, tileVFS,
			m_ComplexIntermediate[0]->GetFrameBuffer(), m_ComplexIntermediate[1]->GetFrameBuffer(),
			newTileCoC->GetFrameBuffer(), nullptr, meanFB, m_NearVerticalPass, true))
	{
		Cleanup();
		m_NearTileFallbackReason = "Near Tile H/V pipeline init failed";
		return false;
	}

	if (m_NearTileCoC || m_NearTileMaxPipeline || m_NearTileHorizontalPipeline ||
		m_NearTileVerticalPipeline || m_PreprocessComputeTilePipeline)
	{
		RetiredWorkingResources retired;
		retired.completionFence = m_LastFrameFence;
		if (m_NearTileCoC)
		{
			retired.targets.push_back(std::move(m_NearTileCoC));
		}
		if (m_NearTileMaxPipeline)
		{
			retired.computePipelines.push_back(std::move(m_NearTileMaxPipeline));
		}
		if (m_NearTileHorizontalPipeline)
		{
			retired.pipelines.push_back(std::move(m_NearTileHorizontalPipeline));
		}
		if (m_NearTileVerticalPipeline)
		{
			retired.pipelines.push_back(std::move(m_NearTileVerticalPipeline));
		}
		if (m_PreprocessComputeTilePipeline)
		{
			retired.computePipelines.push_back(std::move(m_PreprocessComputeTilePipeline));
		}
		m_ComputePreprocessTileReady = false;
		m_RetiredWorkingResources.push_back(std::move(retired));
		ReleaseRetiredWorkingResources();
	}

	m_NearTileCoC = std::move(newTileCoC);
	m_NearTileMaxPipeline = std::move(newTileMaxPipeline);
	m_NearTileHorizontalPipeline = std::move(newTileHPipeline);
	m_NearTileVerticalPipeline = std::move(newTileVPipeline);
	m_CompositePipeline->SetSampler(SHADER_BINDING_TEXTURE4, m_NearTileCoC->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
	m_TileNeedsInitialTransition = true;
	m_NearTileReady = true;
	m_NearTileFallbackReason.clear();
	return true;
}

bool KDepthOfField::EnsureComputePreprocessPipeline(bool withTile)
{
	if (!m_ComputePreprocessSupported)
	{
		m_ComputePreprocessFallbackReason = "RGBA16F color/storage/sampled image unsupported; fallback to PS";
		return false;
	}
	if (!m_NearColor || !m_FarColor)
	{
		m_ComputePreprocessFallbackReason = "Working Near/Far targets not initialized; fallback to PS";
		return false;
	}
	if (withTile && (!m_NearTileReady || !m_NearTileCoC))
	{
		m_ComputePreprocessFallbackReason = "Near TileMAX target not ready for fused CS; fallback to PS";
		return false;
	}
	if ((!withTile && m_ComputePreprocessReady && m_PreprocessComputePipeline) ||
		(withTile && m_ComputePreprocessTileReady && m_PreprocessComputeTilePipeline))
	{
		return true;
	}

	IKComputePipelinePtr newPipeline;
	KRenderGlobal::RenderDevice->CreateComputePipeline(newPipeline);
	if (!newPipeline)
	{
		m_ComputePreprocessFallbackReason = "Compute preprocess pipeline allocation failed; fallback to PS";
		return false;
	}
	IKSamplerPtr sampler = KRenderGlobal::GBuffer.GetSampler();
	newPipeline->BindSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), sampler, true);
	newPipeline->BindSampler(SHADER_BINDING_TEXTURE1, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), sampler, true);
	newPipeline->BindStorageImage(SHADER_BINDING_TEXTURE2, m_NearColor->GetFrameBuffer(), PF_R16G16B16A16_FLOAT, COMPUTE_RESOURCE_OUT, 0, true);
	newPipeline->BindStorageImage(SHADER_BINDING_TEXTURE3, m_FarColor->GetFrameBuffer(), PF_R16G16B16A16_FLOAT, COMPUTE_RESOURCE_OUT, 0, true);

	KShaderCompileEnvironment env;
	if (withTile)
	{
		// 关闭 Tile permutation 时不定义 DOF_NEAR_TILE，也不声明 Tile storage descriptor。
		env.macros.push_back({ "DOF_NEAR_TILE", "1" });
		newPipeline->BindStorageImage(SHADER_BINDING_TEXTURE4, m_NearTileCoC->GetFrameBuffer(), PF_R16_FLOAT, COMPUTE_RESOURCE_OUT, 0, true);
	}
	newPipeline->BindStorageBuffer(SHADER_BINDING_TEXTURE5, m_AutoFocusBuffer, COMPUTE_RESOURCE_IN, false);
	newPipeline->BindUniformBuffer(CBT_CAMERA, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA));
	newPipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);

	if (!newPipeline->Init("dof/preprocess.comp", env, false))
	{
		SAFE_UNINIT(newPipeline);
		m_ComputePreprocessFallbackReason = withTile
			? "Fused CS Preprocess+TileMAX init failed; fallback to PS"
			: "CS Preprocess init failed; fallback to PS";
		return false;
	}

	IKComputePipelinePtr& targetPipeline = withTile ? m_PreprocessComputeTilePipeline : m_PreprocessComputePipeline;
	bool& targetReady = withTile ? m_ComputePreprocessTileReady : m_ComputePreprocessReady;
	if (targetPipeline)
	{
		RetiredWorkingResources retired;
		retired.completionFence = m_LastFrameFence;
		retired.computePipelines.push_back(std::move(targetPipeline));
		m_RetiredWorkingResources.push_back(std::move(retired));
		ReleaseRetiredWorkingResources();
	}
	targetPipeline = std::move(newPipeline);
	targetReady = true;
	m_ComputePreprocessFallbackReason.clear();
	return true;
}

bool KDepthOfField::ApplySettings()
{
	if (!m_NearColor || !m_InputWidth || !m_InputHeight)
	{
		return false;
	}
	ReleaseRetiredWorkingResources();
	bool allSucceeded = true;

	m_WorkingResolution = m_WorkingResolution == 1 ? 1 : 0;
	const uint32_t targetDownsample = m_WorkingResolution == 1 ? 4U : 2U;

	// 1. 判定 Bracketing 目标状态、硬件能力及上游非负输入契约。
	bool desiredBracketed = false;
	if (!m_BracketedEnable)
	{
		m_LastBracketedAttemptFailed = false;
		m_BracketedFallbackReason.clear();
	}
	else if (!CanUseBracketed(m_BracketedFallbackReason))
	{
		allSucceeded = false;
	}
	else if (!m_LastBracketedAttemptFailed)
	{
		desiredBracketed = true;
	}
	else
	{
		allSucceeded = false;
	}

	// 2. 分辨率或 Bracketing 变更需要安全帧边界重建工作 RT / Pass / Pipeline。
	// 仅 PS/CS Preprocess 切换共用联合 usage 的 Near/Far RT，不触发重建。
	const bool hadTile = m_NearTileReady;
	const bool needRebuild =
		targetDownsample != m_DownsampleFactor ||
		desiredBracketed != m_BracketedActive;
	if (needRebuild)
	{
		if (!RebuildWorkingResources(m_InputWidth, m_InputHeight, targetDownsample, desiredBracketed, true))
		{
			if (desiredBracketed)
			{
				m_LastBracketedAttemptFailed = true;
				m_BracketedFallbackReason = "Bracketing 5-MRT initialization failed; fallback to baseline FP16";
				KG_LOGW(LM_RENDER, "%s", m_BracketedFallbackReason.c_str());
				allSucceeded = false;
				// 若同时切了分辨率，则回退到非 Bracketing 完成分辨率切换。
				if (targetDownsample != m_DownsampleFactor || m_BracketedActive)
				{
					if (!RebuildWorkingResources(m_InputWidth, m_InputHeight, targetDownsample, false, true))
					{
						return false;
					}
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			m_LastBracketedAttemptFailed = false;
			m_LastTileAttemptFailed = false;
			m_LastComputeAttemptFailed = false;
			m_LastComputeTileAttemptFailed = false;
		}
		// 若此前已启用过 Tile 资源且当前仍启用 Tile，则随新工作尺寸/Bracketing 模式一并重建。
		if (hadTile && m_NearTileEnable && m_NearTileSupported)
		{
			if (!InitializeNearTile())
			{
				m_NearTileActive = false;
				m_LastTileAttemptFailed = true;
				KG_LOGW(LM_RENDER, "Near TileMAX rebuild failed (%s); use baseline convolution", m_NearTileFallbackReason.c_str());
				allSucceeded = false;
			}
		}
	}

	// 3. 应用 Near TileMAX 请求状态。
	if (!m_NearTileEnable)
	{
		m_NearTileActive = false;
		m_LastTileAttemptFailed = false;
		m_NearTileFallbackReason.clear();
	}
	else if (!m_NearTileSupported)
	{
		m_NearTileActive = false;
		m_NearTileFallbackReason = "R16F storage/sampled image unsupported";
		allSucceeded = false;
	}
	else if (m_NearTileReady)
	{
		m_NearTileActive = true;
		m_NearTileFallbackReason.clear();
	}
	else if (!m_LastTileAttemptFailed)
	{
		if (InitializeNearTile())
		{
			m_NearTileActive = true;
		}
		else
		{
			m_NearTileActive = false;
			m_LastTileAttemptFailed = true;
			KG_LOGW(LM_RENDER, "Near TileMAX unavailable (%s); use the baseline convolution", m_NearTileFallbackReason.c_str());
			allSucceeded = false;
		}
	}
	else
	{
		m_NearTileActive = false;
		allSucceeded = false;
	}

	// 4. 应用 Compute Preprocess 请求状态（不重分配 Near/Far RT，按需惰性构建对应 CS pipeline）。
	if (!m_UseComputePreprocess)
	{
		m_ComputePreprocessActive = false;
		m_LastComputeAttemptFailed = false;
		m_LastComputeTileAttemptFailed = false;
		m_ComputePreprocessFallbackReason.clear();
	}
	else if (!m_ComputePreprocessSupported)
	{
		m_ComputePreprocessActive = false;
		m_ComputePreprocessFallbackReason = "RGBA16F color/storage/sampled image unsupported; fallback to PS";
		allSucceeded = false;
	}
	else
	{
		const bool withTile = m_NearTileActive;
		bool& failedFlag = withTile ? m_LastComputeTileAttemptFailed : m_LastComputeAttemptFailed;
		const bool alreadyReady = withTile ? m_ComputePreprocessTileReady : m_ComputePreprocessReady;
		if (alreadyReady)
		{
			m_ComputePreprocessActive = true;
			m_ComputePreprocessFallbackReason.clear();
		}
		else if (!failedFlag)
		{
			if (EnsureComputePreprocessPipeline(withTile))
			{
				m_ComputePreprocessActive = true;
			}
			else
			{
				m_ComputePreprocessActive = false;
				failedFlag = true;
				KG_LOGW(LM_RENDER, "Compute Preprocess unavailable (%s)", m_ComputePreprocessFallbackReason.c_str());
				allSucceeded = false;
			}
		}
		else
		{
			m_ComputePreprocessActive = false;
			allSucceeded = false;
		}
	}

	return allSucceeded;
}

bool KDepthOfField::Reload()
{
	for (KShaderRef* shader : {
		&m_QuadVS, &m_PreprocessFS, &m_CompositeFS,
		&m_NearHorizontalFS, &m_NearVerticalFS, &m_FarHorizontalFS, &m_FarVerticalFS,
		&m_NearBracketedHorizontalFS, &m_NearBracketedVerticalFS,
		&m_FarBracketedHorizontalFS, &m_FarBracketedVerticalFS,
		&m_NearTileHorizontalFS, &m_NearTileVerticalFS,
		&m_NearTileBracketedHorizontalFS, &m_NearTileBracketedVerticalFS })
	{
		if (*shader)
		{
			(*shader)->Reload();
		}
	}
	for (IKPipelinePtr* pipeline : {
		&m_PreprocessPipeline, &m_NearHorizontalPipeline, &m_NearVerticalPipeline,
		&m_FarHorizontalPipeline, &m_FarVerticalPipeline, &m_CompositePipeline,
		&m_NearTileHorizontalPipeline, &m_NearTileVerticalPipeline })
	{
		if (*pipeline)
		{
			(*pipeline)->Reload(false);
		}
	}
	for (IKComputePipelinePtr* compute : {
		&m_AutoFocusPipeline, &m_NearTileMaxPipeline, &m_PreprocessComputePipeline, &m_PreprocessComputeTilePipeline })
	{
		if (*compute)
		{
			(*compute)->Reload();
		}
	}

	m_LastTileAttemptFailed = false;
	m_LastComputeAttemptFailed = false;
	m_LastComputeTileAttemptFailed = false;
	m_LastBracketedAttemptFailed = false;
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

	const bool hadTileReady = m_NearTileReady;
	const bool hadComputeActive = m_ComputePreprocessActive;

	if (!RebuildWorkingResources(width, height, m_DownsampleFactor, m_BracketedActive, false))
	{
		return false;
	}

	if (m_AutoFocusPipeline)
	{
		m_AutoFocusPipeline->BindSampler(
			SHADER_BINDING_TEXTURE0,
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
	}

	if (m_NearTileEnable && !m_NearTileSupported)
	{
		m_NearTileActive = false;
		m_NearTileFallbackReason = "R16F storage/sampled image unsupported";
	}
	else if (hadTileReady || m_NearTileEnable)
	{
		if (!InitializeNearTile())
		{
			m_NearTileActive = false;
			m_LastTileAttemptFailed = true;
		}
		else
		{
			m_NearTileActive = m_NearTileEnable;
		}
	}

	if (m_UseComputePreprocess && !m_ComputePreprocessSupported)
	{
		m_ComputePreprocessActive = false;
		m_ComputePreprocessFallbackReason = "RGBA16F color/storage/sampled image unsupported; fallback to PS";
	}
	else if (hadComputeActive || m_UseComputePreprocess)
	{
		if (!EnsureComputePreprocessPipeline(m_NearTileActive))
		{
			m_ComputePreprocessActive = false;
			m_LastComputeTileAttemptFailed = m_NearTileActive;
			m_LastComputeAttemptFailed = !m_NearTileActive;
		}
		else
		{
			m_ComputePreprocessActive = m_UseComputePreprocess;
		}
	}

	return true;
}

bool KDepthOfField::DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	return m_DebugDrawer.Render(renderPass, commandList);
}

bool KDepthOfField::Execute(KRHICommandList& commandList)
{
	const KCamera* camera = KRenderGlobal::Renderer.GetCamera();
	const bool wantAutoFocus = (m_FocusMode == KDOFFocusMode::AutoFocusGPU);
	bool validParams =
		std::isfinite(m_FStop) && m_FStop > 0.0f &&
		std::isfinite(m_FocalLength) && m_FocalLength > 0.0f &&
		(wantAutoFocus || (std::isfinite(m_FocusDistance) && m_FocusDistance > m_FocalLength)) &&
		m_InputWidth > 0 && m_InputHeight > 0 && m_Width > 0 && m_Height > 0 &&
		m_InputWidth == KRenderGlobal::GBuffer.GetWidth() &&
		m_InputHeight == KRenderGlobal::GBuffer.GetHeight() &&
		camera && camera->GetProjectiveMode() == KCamera::PM_PERSPECTIVE;

	float opticalCoCScale = 0.0f;
	float radiusScaleFull = 0.0f;
	float apertureTimesFocal = 0.0f;
	float pixelsPerUnit = 0.0f;
	float minFocusF = 0.0f;
	float maxFocusF = 0.0f;
	float initialManualS = 0.0f;
	bool useAutoFocus = false;

	if (!wantAutoFocus)
	{
		m_AutoFocusFallbackReason.clear();
	}

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
			const double afTerm = aperture * focalLength;
			const double ppuTerm = 0.5 * double(m_InputHeight) / sensorHeight;
			apertureTimesFocal = float(afTerm);
			pixelsPerUnit = float(ppuTerm);
			validParams = std::isfinite(apertureTimesFocal) && apertureTimesFocal > 0.0f &&
				std::isfinite(pixelsPerUnit) && pixelsPerUnit > 0.0f;

			if (validParams && !wantAutoFocus)
			{
				opticalCoCScale = float(afTerm / (double(m_FocusDistance) - focalLength));
				// 0.5 将光学 CoC 直径变成半径，与后续工作分辨率降采样无关。
				radiusScaleFull = float((afTerm / (double(m_FocusDistance) - focalLength)) * ppuTerm);
				validParams = std::isfinite(opticalCoCScale) && opticalCoCScale > 0.0f &&
					std::isfinite(radiusScaleFull) && radiusScaleFull > 0.0f;
			}
			else if (validParams && wantAutoFocus)
			{
				if (!m_AutoFocusReady || !m_AutoFocusBuffer || !m_AutoFocusPipeline)
				{
					m_AutoFocusFallbackReason = "AutoFocus compute resources unavailable";
					validParams = false;
				}
				else
				{
					const KDOFAutoFocusSettings& af = m_AutoFocusSettings;
					const bool settingsFinite =
						std::isfinite(af.focusUV.x) && std::isfinite(af.focusUV.y) &&
						std::isfinite(af.sampleStride1080P) && af.sampleStride1080P > 0.0f &&
						std::isfinite(af.smoothTime) &&
						std::isfinite(af.startThreshold) && std::isfinite(af.snapThreshold) &&
						af.snapThreshold > 0.0f && af.startThreshold > af.snapThreshold &&
						std::isfinite(af.minFocusDistance) && af.minFocusDistance >= 0.0f &&
						std::isfinite(af.maxFocusDistance) && af.maxFocusDistance >= 0.0f &&
						std::isfinite(m_FrameDeltaTime) && m_FrameDeltaTime >= 0.0f;
					if (!settingsFinite)
					{
						m_AutoFocusFallbackReason = "Invalid AutoFocus thresholds, range, or deltaTime";
						validParams = false;
					}
					else
					{
						const double margin = std::max(1e-3, 1e-4 * std::max(double(nearPlane), focalLength));
						const double effectiveMin = std::max(
							double(af.minFocusDistance),
							std::max(double(nearPlane) + margin, focalLength + margin));
						const double effectiveMax = af.maxFocusDistance > 0.0f
							? double(af.maxFocusDistance)
							: std::max(effectiveMin, double(farPlane));
						minFocusF = float(effectiveMin);
						if (!(minFocusF > m_FocalLength))
						{
							minFocusF = std::nextafter(m_FocalLength, std::numeric_limits<float>::infinity());
						}
						maxFocusF = float(effectiveMax);
						if (!std::isfinite(minFocusF) || !std::isfinite(maxFocusF) || !(minFocusF > m_FocalLength) ||
							!(maxFocusF >= minFocusF) || !std::isfinite(1.0f / minFocusF) || !std::isfinite(1.0f / maxFocusF))
						{
							m_AutoFocusFallbackReason = "Invalid AutoFocus focus distance range (maxFocus < minFocus)";
							validParams = false;
						}
						else
						{
							initialManualS = std::isfinite(m_FocusDistance)
								? std::max(minFocusF, std::min(m_FocusDistance, maxFocusF))
								: minFocusF;
							useAutoFocus = m_Enable;
							m_AutoFocusFallbackReason.clear();
						}
					}
				}
			}
		}
	}

	if (validParams)
	{
		m_InvalidParamsLogged = false;
	}
	else
	{
		opticalCoCScale = radiusScaleFull = 0.0f;
		useAutoFocus = false;
		if (!m_InvalidParamsLogged)
		{
			KG_LOGW(LM_RENDER, "Invalid DOF optics, projection, AutoFocus settings or resolution; disable blur until valid");
			m_InvalidParamsLogged = true;
		}
	}

	if (wantAutoFocus && m_PrevFocusMode != KDOFFocusMode::AutoFocusGPU)
	{
		// Manual -> AutoFocusGPU：默认从当前手动 FocusDistance 初始化历史并置为 Idle。
		++m_ManualResetGeneration;
	}
	m_PrevFocusMode = m_FocusMode;
	m_AutoFocusActive = useAutoFocus;

	uint32_t autoFocusViewIndex = 0;
	uint32_t autoFocusResetMode = DOF_AUTOFOCUS_RESET_NONE;
	if (useAutoFocus)
	{
		autoFocusViewIndex = AcquireAutoFocusViewSlot(camera, autoFocusResetMode);
	}

	struct ObjectData
	{
		// 对焦距离、光学 CoC 直径系数、全分辨率像素半径系数、工作/全分辨率像素尺度。
		glm::vec4 dofParams;
		// 参数有效标志、诊断视图（0 为最终图像）、Tile 生效标志、AutoFocus 槽位索引（0 为 Manual，>=1 为 viewIndex+1）。
		glm::vec4 dofParams2;
	} objectData = {};

	// 使用整数因子的倒数，而非取整后的 workHeight/fullHeight，避免 padding 改变光学尺度。
	objectData.dofParams = glm::vec4(validParams ? m_FocusDistance : 0.0f, opticalCoCScale, radiusScaleFull,
		1.0f / float(m_DownsampleFactor));
	const int debugView = m_DebugView >= DV_FINAL && m_DebugView < DV_COUNT ? m_DebugView : DV_FINAL;
	const bool useTile = m_Enable && m_NearTileActive && m_NearTileReady && bool(m_NearTileCoC);
	const bool useCompute = m_Enable && m_ComputePreprocessActive &&
		(useTile ? (m_ComputePreprocessTileReady && bool(m_PreprocessComputeTilePipeline))
		         : (m_ComputePreprocessReady && bool(m_PreprocessComputePipeline)));
	const bool useBracketed = m_BracketedActive && bool(m_HorizontalColorMean);
	objectData.dofParams2 = glm::vec4(
		m_Enable && validParams ? 1.0f : 0.0f,
		float(debugView),
		useTile ? 1.0f : 0.0f,
		useAutoFocus ? float(autoFocusViewIndex + 1u) : 0.0f);

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

	const PipelineStages sceneReadStages = useCompute
		? (PIPELINE_STAGE_FRAGMENT_SHADER | PIPELINE_STAGE_COMPUTE_SHADER)
		: PIPELINE_STAGE_FRAGMENT_SHADER;

	commandList.BeginDebugMarker("DOF", glm::vec4(1));
	commandList.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
		PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER | PIPELINE_STAGE_COMPUTE_SHADER,
		IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(),
		PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, sceneReadStages,
		IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

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
	if (m_HorizontalColorMean && m_ColorMeanNeedsInitialTransition)
	{
		InitializeReadOnly(m_HorizontalColorMean);
		m_ColorMeanNeedsInitialTransition = false;
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
		if (useAutoFocus)
		{
			struct AutoFocusObjectData
			{
				glm::vec4 sampleAndTime;
				glm::vec4 controlParams;
				glm::vec4 opticsParams;
				glm::vec4 viewAndReset;
			} afData = {};
			afData.sampleAndTime = glm::vec4(
				m_AutoFocusSettings.focusUV.x,
				m_AutoFocusSettings.focusUV.y,
				m_AutoFocusSettings.sampleStride1080P,
				m_FrameDeltaTime);
			afData.controlParams = glm::vec4(
				m_AutoFocusSettings.smoothTime,
				m_AutoFocusSettings.startThreshold,
				m_AutoFocusSettings.snapThreshold,
				initialManualS);
			afData.opticsParams = glm::vec4(
				minFocusF,
				maxFocusF,
				m_FocalLength,
				apertureTimesFocal);
			afData.viewAndReset = glm::vec4(
				pixelsPerUnit,
				1.0f / float(m_DownsampleFactor),
				float(autoFocusViewIndex),
				float(autoFocusResetMode));

			// 覆盖跨帧 WAR（上帧 Composite/Preprocess 读 -> 本帧 AutoFocus 写）及 RAW/WAW（上帧 AutoFocus 写 -> 本帧读写）。
			commandList.PipelineMemoryBarrier(
				PIPELINE_STAGE_COMPUTE_SHADER | PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COMPUTE_SHADER);
			commandList.BeginDebugMarker("DOF_AutoFocus", glm::vec4(1));
			KDynamicConstantBufferUsage afUsage;
			afUsage.binding = SHADER_BINDING_OBJECT;
			afUsage.range = sizeof(afData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&afData, afUsage);
			commandList.Execute(m_AutoFocusPipeline, 1, 1, 1, &afUsage);
			commandList.EndDebugMarker();
			// 覆盖同帧 RAW（本帧 AutoFocus 写 -> 本帧 Preprocess CS/PS、TileMAX CS、Composite PS 读）。
			commandList.PipelineMemoryBarrier(
				PIPELINE_STAGE_COMPUTE_SHADER,
				PIPELINE_STAGE_COMPUTE_SHADER | PIPELINE_STAGE_FRAGMENT_SHADER);
		}

		const uint32_t tileGroupsX = (m_Width + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
		const uint32_t tileGroupsY = (m_Height + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;

		if (useCompute)
		{
			// CS Preprocess：将共享的 Near/Far 从跨帧只读布局转为 GENERAL 供 imageStore 写入。
			commandList.Transition(m_NearColor->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
			commandList.Transition(m_FarColor->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
			if (useTile)
			{
				commandList.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
					PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
			}

			commandList.BeginDebugMarker(useTile ? "DOF_PreprocessCS_TileMAX" : "DOF_PreprocessCS", glm::vec4(1));
			KDynamicConstantBufferUsage preprocessUsage;
			preprocessUsage.binding = SHADER_BINDING_OBJECT;
			preprocessUsage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, preprocessUsage);
			commandList.Execute(useTile ? m_PreprocessComputeTilePipeline : m_PreprocessComputePipeline,
				tileGroupsX, tileGroupsY, 1, &preprocessUsage);
			commandList.EndDebugMarker();

			// CS 写完后转回 SHADER_READ_ONLY，与后续 Graphics 采样描述符布局严格一致。
			commandList.Transition(m_NearColor->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
			commandList.Transition(m_FarColor->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
			if (useTile)
			{
				commandList.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
					PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
			}
		}
		else
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
				commandList.Execute(m_NearTileMaxPipeline, tileGroupsX, tileGroupsY, 1, &tileUsage);
				commandList.EndDebugMarker();
				commandList.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
					PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
			}
		}

		ToColorAttachment(m_ComplexIntermediate[0]);
		ToColorAttachment(m_ComplexIntermediate[1]);
		if (useBracketed)
		{
			ToColorAttachment(m_HorizontalColorMean);
		}
		DrawPass("DOF_NearHorizontal", m_NearHorizontalPass,
			useTile ? m_NearTileHorizontalPipeline : m_NearHorizontalPipeline, false);
		ToReadOnly(m_ComplexIntermediate[0]);
		ToReadOnly(m_ComplexIntermediate[1]);
		if (useBracketed)
		{
			ToReadOnly(m_HorizontalColorMean);
		}

		ToColorAttachment(m_NearColor);
		DrawPass("DOF_NearVertical", m_NearVerticalPass,
			useTile ? m_NearTileVerticalPipeline : m_NearVerticalPipeline, false);
		ToReadOnly(m_NearColor);

		// Near V 已读完前两张复数纹理及 ColorMean，此后 Far H 可覆盖全部四张及 ColorMean；跨帧仍回到只读布局。
		for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
		{
			ToColorAttachment(intermediate);
		}
		if (useBracketed)
		{
			ToColorAttachment(m_HorizontalColorMean);
		}
		DrawPass("DOF_FarHorizontal", m_FarHorizontalPass, m_FarHorizontalPipeline, false);
		for (IKRenderTargetPtr& intermediate : m_ComplexIntermediate)
		{
			ToReadOnly(intermediate);
		}
		if (useBracketed)
		{
			ToReadOnly(m_HorizontalColorMean);
		}

		ToColorAttachment(m_FarColor);
		DrawPass("DOF_FarVertical", m_FarVerticalPass, m_FarVerticalPipeline, false);
		ToReadOnly(m_FarColor);
	}

	ToColorAttachment(m_Final);
	DrawPass("DOF_Composite", m_CompositePass, m_CompositePipeline, true);
	ToReadOnly(m_Final);

	commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(),
		sceneReadStages, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
		IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	commandList.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
		PIPELINE_STAGE_FRAGMENT_SHADER | PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
		IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	commandList.EndDebugMarker();

	return true;
}
