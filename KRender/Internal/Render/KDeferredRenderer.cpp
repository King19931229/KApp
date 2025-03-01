#include "KDeferredRenderer.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSampler.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKStatistics.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Render/KRenderUtil.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "KBase/Interface/IKLog.h"

struct KDeferredRenderStageDescription
{
	DeferredRenderStage stage;
	RenderStage renderStage;
	RenderStage instanceRenderStage;
	bool allowGPUScene;
	bool allowMultithread;
	const char* debugMarker;
};

constexpr KDeferredRenderStageDescription GDeferredRenderStageDescription[DRS_STAGE_COUNT] =
{
	{DRS_STAGE_PRE_PASS, RENDER_STAGE_PRE_Z, RENDER_STAGE_PRE_Z_INSTANCE, false, false, "PrePass"},
	{DRS_STAGE_MAIN_BASE_PASS, RENDER_STAGE_BASEPASS, RENDER_STAGE_BASEPASS_INSTANCE, true, true, "MainBasePass"},
	{DRS_STAGE_POST_BASE_PASS, RENDER_STAGE_BASEPASS, RENDER_STAGE_BASEPASS_INSTANCE, true, true, "PostBasePass" },
	{DRS_STAGE_DEFERRED_LIGHTING, RENDER_STAGE_UNKNOWN, RENDER_STAGE_UNKNOWN, false, false, "LightingPass"},
	{DRS_STAGE_FORWARD_OPAQUE, RENDER_STAGE_OPAQUE, RENDER_STAGE_UNKNOWN, true, true, "ForwardOpaquePass"},
	{DRS_STATE_COPY_OPAQUE_COLOR, RENDER_STAGE_UNKNOWN, RENDER_STAGE_UNKNOWN, false, false, "CopyOpaqueColor"},
	{DRS_STAGE_FORWARD_TRANSPRANT, RENDER_STAGE_TRANSPRANT, RENDER_STAGE_UNKNOWN, false, false, "ForwardTransprantPass"},
	{DRS_STATE_SKY, RENDER_STAGE_UNKNOWN, RENDER_STAGE_UNKNOWN, false, false, "SkyPass"},
	{DRS_STATE_POSTPROCESS_RESULT, RENDER_STAGE_UNKNOWN, RENDER_STAGE_UNKNOWN, false, false, "PostProcessResult"},
	{DRS_STATE_DEBUG_OBJECT, RENDER_STAGE_UNKNOWN, RENDER_STAGE_UNKNOWN, false, false, "DebugObjectPass"},
	{DRS_STATE_FOREGROUND, RENDER_STAGE_UNKNOWN, RENDER_STAGE_UNKNOWN, false, false, "ForegroundPass"}
};

static_assert(GDeferredRenderStageDescription[DRS_STAGE_PRE_PASS].stage == DRS_STAGE_PRE_PASS, "check");
static_assert(GDeferredRenderStageDescription[DRS_STAGE_MAIN_BASE_PASS].stage == DRS_STAGE_MAIN_BASE_PASS, "check");
static_assert(GDeferredRenderStageDescription[DRS_STAGE_POST_BASE_PASS].stage == DRS_STAGE_POST_BASE_PASS, "check");
static_assert(GDeferredRenderStageDescription[DRS_STAGE_DEFERRED_LIGHTING].stage == DRS_STAGE_DEFERRED_LIGHTING, "check");
static_assert(GDeferredRenderStageDescription[DRS_STAGE_FORWARD_TRANSPRANT].stage == DRS_STAGE_FORWARD_TRANSPRANT, "check");
static_assert(GDeferredRenderStageDescription[DRS_STAGE_FORWARD_OPAQUE].stage == DRS_STAGE_FORWARD_OPAQUE, "check");
static_assert(GDeferredRenderStageDescription[DRS_STATE_SKY].stage == DRS_STATE_SKY, "check");
static_assert(GDeferredRenderStageDescription[DRS_STATE_POSTPROCESS_RESULT].stage == DRS_STATE_POSTPROCESS_RESULT, "check");
static_assert(GDeferredRenderStageDescription[DRS_STATE_DEBUG_OBJECT].stage == DRS_STATE_DEBUG_OBJECT, "check");
static_assert(GDeferredRenderStageDescription[DRS_STATE_FOREGROUND].stage == DRS_STATE_FOREGROUND, "check");

static_assert(ARRAY_SIZE(GDeferredRenderDebugDescription) == DRD_COUNT, "check");

KDeferredRenderer::KDeferredRenderer()
	: m_Camera(nullptr)
	, m_DebugOption(DRD_NONE)
{
}

KDeferredRenderer::~KDeferredRenderer()
{
}

void KDeferredRenderer::Resize(uint32_t width, uint32_t height)
{
	RecreateRenderPass(width, height);
	RecreatePipeline();
}

void KDeferredRenderer::Init(const KCamera* camera, uint32_t width, uint32_t height)
{
	UnInit();

	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	for (uint32_t i = 0; i < DRS_STAGE_COUNT; ++i)
	{
		KRenderGlobal::Statistics.RegisterRenderStage(GDeferredRenderStageDescription[i].debugMarker);
	}

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shading/deferred.frag", m_DeferredLightingFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shading/draw.frag", m_SceneColorDrawFS, false);
	
	renderDevice->CreateRenderTarget(m_FinalTarget);

	RecreateRenderPass(width, height);
	RecreatePipeline();

	m_Camera = camera;
}

void KDeferredRenderer::UnInit()
{
	m_QuadVS.Release();
	m_DeferredLightingFS.Release();
	m_SceneColorDrawFS.Release();

	SAFE_UNINIT(m_FinalTarget);
	SAFE_UNINIT(m_LightingPipeline);
	SAFE_UNINIT(m_DrawSceneColorPipeline);
	SAFE_UNINIT(m_DrawProcessResultPipeline);
	SAFE_UNINIT(m_DrawFinalPipeline);

	SAFE_UNINIT(m_EmptyAORenderPass);

	for (uint32_t i = 0; i < DRS_STAGE_COUNT; ++i)
	{
		SAFE_UNINIT(m_RenderPass[i]);
		m_RenderCallFuncs[i].clear();
		KRenderGlobal::Statistics.UnRegisterRenderStage(GDeferredRenderStageDescription[i].debugMarker);
	}
	m_Camera = nullptr;
}

void KDeferredRenderer::AddCallFunc(DeferredRenderStage stage, RenderPassCallFuncType* func)
{
	auto it = std::find(m_RenderCallFuncs[stage].begin(), m_RenderCallFuncs[stage].end(), func);
	if (it == m_RenderCallFuncs[stage].end())
	{
		m_RenderCallFuncs[stage].push_back(func);
	}
}

void KDeferredRenderer::RemoveCallFunc(DeferredRenderStage stage, RenderPassCallFuncType* func)
{
	auto it = std::find(m_RenderCallFuncs[stage].begin(), m_RenderCallFuncs[stage].end(), func);
	if (it != m_RenderCallFuncs[stage].end())
	{
		m_RenderCallFuncs[stage].erase(it);
	}
}

void KDeferredRenderer::RecreateRenderPass(uint32_t width, uint32_t height)
{
	m_FinalTarget->UnInit();
	m_FinalTarget->InitFromColor(width, height, 1, 1, EF_R16G16B16A16_FLOAT);
	m_FinalTarget->GetFrameBuffer()->SetDebugName("FinalTarget");

	auto EnsureRenderPass = [](IKRenderPassPtr& renderPass)
	{
		if (renderPass) renderPass->UnInit();
		else ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(renderPass));
	};

	EnsureRenderPass(m_EmptyAORenderPass);
	m_EmptyAORenderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetAOTarget()->GetFrameBuffer());
	m_EmptyAORenderPass->SetOpColor(0, LO_CLEAR, SO_STORE);
	m_EmptyAORenderPass->SetClearColor(0, { 1.0f, 1.0f, 1.0f, 1.0f });
	ASSERT_RESULT(m_EmptyAORenderPass->Init());

	for (uint32_t idx = 0; idx < DRS_STAGE_COUNT; ++idx)
	{
		IKRenderPassPtr& renderPass = m_RenderPass[idx];
		EnsureRenderPass(renderPass);

		if (idx == DRS_STAGE_MAIN_BASE_PASS || idx == DRS_STAGE_POST_BASE_PASS)
		{
			LoadOperation loadOp = (idx == DRS_STAGE_MAIN_BASE_PASS) ? LO_CLEAR : LO_LOAD;
			for (uint32_t gbuffer = GBUFFER_TARGET0; gbuffer < GBUFFER_TARGET_COUNT; ++gbuffer)
			{
				renderPass->SetColorAttachment(gbuffer, KRenderGlobal::GBuffer.GetGBufferTarget((GBufferTarget)gbuffer)->GetFrameBuffer());
				renderPass->SetOpColor(gbuffer, loadOp, SO_STORE);
				renderPass->SetClearColor(gbuffer, { 0.0f, 0.0f, 0.0f, 0.0f });
			}

			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			renderPass->SetOpDepthStencil(loadOp, SO_STORE, loadOp, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STAGE_DEFERRED_LIGHTING)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_CLEAR, SO_STORE);
			renderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STAGE_FORWARD_OPAQUE)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STAGE_FORWARD_TRANSPRANT)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_COPY_OPAQUE_COLOR)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetOpaqueColorCopy()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_CLEAR, SO_STORE);
			renderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_SKY)
		{
			renderPass->SetColorAttachment(0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_POSTPROCESS_RESULT)
		{
			renderPass->SetColorAttachment(0, m_FinalTarget->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_CLEAR, SO_STORE);
			renderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_DEBUG_OBJECT)
		{
			renderPass->SetColorAttachment(0, m_FinalTarget->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetOpDepthStencil(LO_LOAD, SO_STORE, LO_LOAD, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}

		if (idx == DRS_STATE_FOREGROUND)
		{
			renderPass->SetColorAttachment(0, m_FinalTarget->GetFrameBuffer());
			renderPass->SetOpColor(0, LO_LOAD, SO_STORE);
			renderPass->SetDepthStencilAttachment(KRenderGlobal::GBuffer.GetDepthStencilTarget()->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			renderPass->SetOpDepthStencil(LO_CLEAR, SO_STORE, LO_CLEAR, SO_STORE);
			ASSERT_RESULT(renderPass->Init());
		}
	}
}

void KDeferredRenderer::RecreatePipeline()
{
	auto EnsurePipeline = [](IKPipelinePtr& pipeline)
	{
		if (pipeline) pipeline->UnInit();
		else ASSERT_RESULT(KRenderGlobal::RenderDevice->CreatePipeline(pipeline));
	};

	{
		EnsurePipeline(m_LightingPipeline);
		IKPipelinePtr& pipeline = m_LightingPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_DeferredLightingFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		IKUniformBufferPtr voxelSVOBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
		pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_FRAGMENT, voxelSVOBuffer);

		IKUniformBufferPtr voxelClipmapBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);
		pipeline->SetConstantBuffer(CBT_VOXEL_CLIPMAP, ST_VERTEX | ST_FRAGMENT, voxelClipmapBuffer);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
		pipeline->SetConstantBuffer(CBT_GLOBAL, ST_VERTEX | ST_FRAGMENT, globalBuffer);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0,
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1,
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET1)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2,
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET2)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE3,
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET3)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE4,
			KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET4)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE5,
			KRenderGlobal::CascadedShadowMap.GetFinalMask()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::SVO_GI)
		{
			pipeline->SetSampler(SHADER_BINDING_TEXTURE6,
				KRenderGlobal::Voxilzer.GetFinalMask()->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);
		}

		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			pipeline->SetSampler(SHADER_BINDING_TEXTURE6,
				KRenderGlobal::ClipmapVoxilzer.GetFinalMask()->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);
		}

		pipeline->SetSampler(SHADER_BINDING_TEXTURE7,
			KRenderGlobal::GBuffer.GetAOTarget()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE8,
			KRenderGlobal::VolumetricFog.GetScatteringTarget()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE9,
			KRenderGlobal::ScreenSpaceReflection.GetAOTarget()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE10,
			KRenderGlobal::CubeMap.GetDiffuseIrradiance()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetDiffuseIrradianceSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE11,
			KRenderGlobal::CubeMap.GetSpecularIrradiance()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetSpecularIrradianceSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE12,
			KRenderGlobal::CubeMap.GetIntegrateBRDF()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetIntegrateBRDFSampler(),
			true);

		pipeline->SetDebugName("LightingPipeline");
		pipeline->Init();
	}

	{
		EnsurePipeline(m_DrawFinalPipeline);
		IKPipelinePtr& pipeline = m_DrawFinalPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_SceneColorDrawFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_FinalTarget->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
	
		pipeline->SetDebugName("DrawFinalPipeline");
		pipeline->Init();
	}

	{
		EnsurePipeline(m_DrawProcessResultPipeline);
		IKPipelinePtr& pipeline = m_DrawProcessResultPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_SceneColorDrawFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		// TODO PostProcessMgr
		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::DepthOfField.GetFinal()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);

		pipeline->SetDebugName("DrawFinalPipeline");
		pipeline->Init();
	}

	{
		EnsurePipeline(m_DrawSceneColorPipeline);
		IKPipelinePtr& pipeline = m_DrawSceneColorPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_SceneColorDrawFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);

		pipeline->SetDebugName("DrawSceneColorPipeline");
		pipeline->Init();
	}
}

void KDeferredRenderer::BuildMaterialSubMeshInstance(DeferredRenderStage renderStage, const std::vector<IKEntity*>& cullRes, std::vector<KMaterialSubMeshInstance>& instances)
{
	if (renderStage == DRS_STAGE_MAIN_BASE_PASS)
	{
		KRenderUtil::CalculateInstancesByMaterial(cullRes, instances);
	}
	else if(renderStage == DRS_STAGE_FORWARD_TRANSPRANT)
	{
		KMaterialSubMeshInstanceCompareFunction comp = [this](const KMaterialSubMeshInstance& lhs, const KMaterialSubMeshInstance& rhs) ->bool
		{
			const glm::vec3& cameraPos = m_Camera->GetPosition();
			const glm::vec3& cameraForward = m_Camera->GetForward();

			assert(lhs.instanceData.size() == rhs.instanceData.size() == 1);
			const glm::vec3 lhsPos = glm::vec3(lhs.instanceData[0].ROW0[3], lhs.instanceData[0].ROW1[3], lhs.instanceData[0].ROW2[3]);
			const glm::vec3 rhsPos = glm::vec3(rhs.instanceData[0].ROW0[3], rhs.instanceData[0].ROW1[3], rhs.instanceData[0].ROW2[3]);

			const float lhsDis = glm::dot(lhsPos - cameraPos, cameraForward);
			const float rhsDis = glm::dot(rhsPos - cameraPos, cameraForward);

			return lhsDis > rhsDis;
		};

		KRenderUtil::GetInstances(cullRes, instances, comp);
	}
}

void KDeferredRenderer::HandleRenderCommandBinding(DeferredRenderStage renderStage, KRenderCommand& command)
{
	if (renderStage == DRS_STAGE_FORWARD_TRANSPRANT)
	{
		IKPipelinePtr& pipeline = command.pipeline;

		IKUniformBufferPtr voxelSVOBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
		pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_FRAGMENT, voxelSVOBuffer);

		IKUniformBufferPtr voxelClipmapBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);
		pipeline->SetConstantBuffer(CBT_VOXEL_CLIPMAP, ST_VERTEX | ST_FRAGMENT, voxelClipmapBuffer);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
		pipeline->SetConstantBuffer(CBT_GLOBAL, ST_VERTEX | ST_FRAGMENT, globalBuffer);

		for (uint32_t cascadedIndex = 0; cascadedIndex <= 3; ++cascadedIndex)
		{
			IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, true);
			if (!shadowRT) shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, true);
			pipeline->SetSampler(SHADER_BINDING_TEXTURE5 + cascadedIndex,
				shadowRT->GetFrameBuffer(),
				KRenderGlobal::CascadedShadowMap.GetSampler(),
				false);
		}
		for (uint32_t cascadedIndex = 0; cascadedIndex <= 3; ++cascadedIndex)
		{
			IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, false);
			if (!shadowRT) shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, false);
			pipeline->SetSampler(SHADER_BINDING_TEXTURE9 + cascadedIndex,
				shadowRT->GetFrameBuffer(),
				KRenderGlobal::CascadedShadowMap.GetSampler(),
				false);
		}

		pipeline->SetSampler(SHADER_BINDING_TEXTURE13,
			KRenderGlobal::CubeMap.GetDiffuseIrradiance()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetDiffuseIrradianceSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE14,
			KRenderGlobal::CubeMap.GetSpecularIrradiance()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetSpecularIrradianceSampler(),
			true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE15,
			KRenderGlobal::CubeMap.GetIntegrateBRDF()->GetFrameBuffer(),
			KRenderGlobal::CubeMap.GetIntegrateBRDFSampler(),
			true);

		/*pipeline->SetSampler(SHADER_BINDING_TEXTURE16,
			KRenderGlobal::DepthPeeling.GetPrevPeelingDepthTarget()->GetFrameBuffer(),
			KRenderGlobal::DepthPeeling.GetPeelingDepthSampler(),
			true);*/

		struct Debug
		{
			uint32_t debugOption;
		} debug;

		debug.debugOption = m_DebugOption;

		KDynamicConstantBufferUsage debugUsage;
		debugUsage.binding = SHADER_BINDING_DEBUG;
		debugUsage.range = sizeof(debug);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&debug, debugUsage);

		command.dynamicConstantUsages.push_back(debugUsage);
	}
}

void KDeferredRenderer::PopulateRenderCommand(DeferredRenderStage deferredRenderStage, const std::vector<IKEntity*>& cullRes, KRenderStageStatistics& statistics, KRenderCommandList& renderCommands)
{
	RenderStage renderStage = GDeferredRenderStageDescription[deferredRenderStage].renderStage;
	RenderStage instanceRenderStage = GDeferredRenderStageDescription[deferredRenderStage].instanceRenderStage;

	renderCommands.clear();

	std::vector<KMaterialSubMeshInstance> subMeshInstances;
	BuildMaterialSubMeshInstance(deferredRenderStage, cullRes, subMeshInstances);

	for (KMaterialSubMeshInstance& subMeshInstance : subMeshInstances)
	{
		const std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F>& instances = subMeshInstance.instanceData;

		ASSERT_RESULT(!instances.empty());

		if (instanceRenderStage != RENDER_STAGE_UNKNOWN && instances.size() > 1)
		{
			KRenderCommand command;
			if (subMeshInstance.materialSubMesh->GetRenderCommand(instanceRenderStage, command))
			{
				if (!KRenderUtil::AssignShadingParameter(command, subMeshInstance.materialSubMesh->GetMaterial()))
				{
					continue;
				}

				if (deferredRenderStage == DRS_STAGE_MAIN_BASE_PASS || deferredRenderStage == DRS_STAGE_POST_BASE_PASS)
				{
					KVirtualTexture* virtualTexture = nullptr;
					if (command.textureBinding->GetIsVirtualTexture(m_CurrentVirtualFeedbackTargetBinding))
					{
						virtualTexture = (KVirtualTexture*)command.textureBinding->GetVirtualTextureSoul(m_CurrentVirtualFeedbackTargetBinding);
					}
					if (!KRenderUtil::AssignVirtualFeedbackParameter(command, m_CurrentVirtualFeedbackTargetBinding, virtualTexture))
					{
						continue;
					}
				}

				std::vector<KInstanceBufferManager::AllocResultBlock> allocRes;
				ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.GetVertexSize() == sizeof(instances[0]));
				ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.Alloc(instances.size(), instances.data(), allocRes));

				command.instanceDraw = true;
				command.instanceUsages.resize(allocRes.size());
				for (size_t i = 0; i < allocRes.size(); ++i)
				{
					KInstanceBufferUsage& usage = command.instanceUsages[i];
					KInstanceBufferManager::AllocResultBlock& allocResult = allocRes[i];
					usage.buffer = allocResult.buffer;
					usage.start = allocResult.start;
					usage.count = allocResult.count;
					usage.offset = allocResult.offset;
				}

				++statistics.drawcalls;

				if (command.indexDraw)
				{
					statistics.primtives += command.indexData->indexCount;
					statistics.faces += command.indexData->indexCount / 3;
				}
				else
				{
					statistics.primtives += command.vertexData->vertexCount;
					statistics.faces += command.vertexData->vertexCount / 3;
				}

				HandleRenderCommandBinding(deferredRenderStage, command);
				command.pipeline->GetHandle(m_RenderPass[deferredRenderStage], command.pipelineHandle);

				if (command.Complete())
				{
					renderCommands.push_back(std::move(command));
				}
			}
		}
		else
		{
			KRenderCommand command;
			if (subMeshInstance.materialSubMesh->GetRenderCommand(renderStage, command))
			{
				if (!KRenderUtil::AssignShadingParameter(command, subMeshInstance.materialSubMesh->GetMaterial()))
				{
					continue;
				}

				if (deferredRenderStage == DRS_STAGE_MAIN_BASE_PASS || deferredRenderStage == DRS_STAGE_POST_BASE_PASS)
				{
					KVirtualTexture* virtualTexture = nullptr;
					if (command.textureBinding->GetIsVirtualTexture(m_CurrentVirtualFeedbackTargetBinding))
					{
						virtualTexture = (KVirtualTexture*)command.textureBinding->GetVirtualTextureSoul(m_CurrentVirtualFeedbackTargetBinding);
					}
					if (virtualTexture && !KRenderUtil::AssignVirtualFeedbackParameter(command, m_CurrentVirtualFeedbackTargetBinding, virtualTexture))
					{
						KG_LOGE(LM_RENDER, "Has virtualTexture but can't AssignVirtualFeedbackParameter");
						continue;
					}
				}

				for (size_t idx = 0; idx < instances.size(); ++idx)
				{
					const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[idx];

					KConstantDefinition::OBJECT objectData;
					objectData.MODEL = glm::transpose(glm::mat4(instance.ROW0, instance.ROW1, instance.ROW2, glm::vec4(0, 0, 0, 1)));
					objectData.PRVE_MODEL = glm::transpose(glm::mat4(instance.PREV_ROW0, instance.PREV_ROW1, instance.PREV_ROW2, glm::vec4(0, 0, 0, 1)));

					KDynamicConstantBufferUsage objectUsage;
					objectUsage.binding = SHADER_BINDING_OBJECT;
					objectUsage.range = sizeof(objectData);
					KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

					command.dynamicConstantUsages.push_back(objectUsage);

					++statistics.drawcalls;

					if (command.indexDraw)
					{
						statistics.primtives += command.indexData->indexCount;
						statistics.faces += command.indexData->indexCount / 3;
					}
					else
					{
						statistics.primtives += command.vertexData->vertexCount;
						statistics.faces += command.vertexData->vertexCount / 3;
					}

					HandleRenderCommandBinding(deferredRenderStage, command);
					command.pipeline->GetHandle(m_RenderPass[deferredRenderStage], command.pipelineHandle);

					if (command.Complete())
					{
						renderCommands.push_back(std::move(command));
					}
				}
			}
		}
	}

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(GDeferredRenderStageDescription[renderStage].debugMarker, statistics);
}

void KDeferredRenderer::ExecuteRenderPass(KRHICommandList& commandList, DeferredRenderStage deferredRenderStage, const std::vector<IKEntity*>& cullRes)
{
	IKRenderPassPtr renderPass = m_RenderPass[deferredRenderStage];

	const char* debugMarker = GDeferredRenderStageDescription[deferredRenderStage].debugMarker;
	commandList.BeginDebugMarker(debugMarker, glm::vec4(1));

	KRenderStageStatistics& statistics = m_Statistics[deferredRenderStage];
	statistics.Reset();

	bool allowGPUScene = GDeferredRenderStageDescription[deferredRenderStage].allowGPUScene;

	KRenderCommandList renderCmdList;
	if (!KRenderGlobal::EnableGPUScene || !allowGPUScene)
	{
		PopulateRenderCommand(deferredRenderStage, cullRes, statistics, renderCmdList);
	}

	bool allowMultithread = GDeferredRenderStageDescription[deferredRenderStage].allowMultithread;
	if (allowMultithread && KRenderGlobal::EnableMultithreadRender)
	{
		commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_SECONDARY);

		uint32_t threadNum = commandList.GetThreadPoolSize();

		uint32_t commandEachThread = (uint32_t)renderCmdList.size() / threadNum;
		uint32_t currentCommandIndex = 0;
		uint32_t currentCommandCount = commandEachThread + renderCmdList.size() % threadNum;

		for (auto it = renderCmdList.begin(); it != renderCmdList.end(); ++it)
		{
			KRenderCommand& command = *it;
			ASSERT_RESULT(command.pipeline->GetHandle(renderPass, command.pipelineHandle));
		}

		threadNum = (commandEachThread > 0) ? threadNum : 1;

		commandList.BeginThreadedRender(threadNum, renderPass, std::move(renderCmdList));
		for (uint32_t threadIndex = 0; threadIndex < threadNum; ++threadIndex)
		{
			commandList.SetThreadedRenderJob(threadIndex, [this, deferredRenderStage, currentCommandIndex, currentCommandCount, threadIndex](KRHICommandList& commandList, IKCommandBufferPtr commandBuffer, IKRenderPassPtr renderPass, KRenderCommandList& renderCmdList)
			{
				commandBuffer->SetViewport(renderPass->GetViewPort());

				for (uint32_t idx = 0; idx < currentCommandCount; ++idx)
				{
					KRenderCommand& command = renderCmdList[currentCommandIndex + idx];
					command.threadIndex = (uint32_t)threadIndex;
					commandBuffer->Render(command);
				}

				if (threadIndex == 0)
				{
					KRHICommandList subCommandList;
					subCommandList.SetCommandBuffer(commandBuffer);
					subCommandList.SetImmediate(true);
					std::for_each(m_RenderCallFuncs[deferredRenderStage].begin(), m_RenderCallFuncs[deferredRenderStage].end(), [renderPass, &subCommandList](RenderPassCallFuncType* func)
					{
						(*func)(renderPass, subCommandList);
					});
				}
			});
			currentCommandIndex += currentCommandCount;
			currentCommandCount = commandEachThread;
		}
		commandList.EndThreadedRender();
	}
	else
	{
		commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(renderPass->GetViewPort());
		for (KRenderCommand& command : renderCmdList)
		{
			IKPipelineHandlePtr handle = nullptr;
			if (command.pipeline->GetHandle(renderPass, handle))
			{
				commandList.Render(command);
			}
		}

		std::for_each(m_RenderCallFuncs[deferredRenderStage].begin(), m_RenderCallFuncs[deferredRenderStage].end(), [renderPass, &commandList](RenderPassCallFuncType* func)
		{
			(*func)(renderPass, commandList);
		});
	}

	commandList.EndRenderPass();
	commandList.EndDebugMarker();
}

void KDeferredRenderer::EmptyAO(KRHICommandList& commandList)
{
	commandList.BeginDebugMarker("EmptyAO", glm::vec4(1));
	commandList.BeginRenderPass(m_EmptyAORenderPass, SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_EmptyAORenderPass->GetViewPort());

	commandList.EndRenderPass();
	commandList.Transition(KRenderGlobal::GBuffer.GetAOTarget()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commandList.EndDebugMarker();
}

void KDeferredRenderer::Foreground(KRHICommandList& commandList)
{
	commandList.BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_FOREGROUND].debugMarker, glm::vec4(1));
	commandList.BeginRenderPass(m_RenderPass[DRS_STATE_FOREGROUND], SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_RenderPass[DRS_STATE_FOREGROUND]->GetViewPort());

	std::for_each(m_RenderCallFuncs[DRS_STATE_FOREGROUND].begin(), m_RenderCallFuncs[DRS_STATE_FOREGROUND].end(), [this, &commandList](RenderPassCallFuncType* func)
	{
		(*func)(m_RenderPass[DRS_STATE_FOREGROUND], commandList);
	});

	commandList.EndRenderPass();
	commandList.EndDebugMarker();
}

void KDeferredRenderer::SkyPass(KRHICommandList& commandList)
{
	commandList.BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_SKY].debugMarker, glm::vec4(1));
	commandList.BeginRenderPass(m_RenderPass[DRS_STATE_SKY], SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_RenderPass[DRS_STATE_SKY]->GetViewPort());

	KRenderGlobal::SkyBox.Render(m_RenderPass[DRS_STATE_SKY], commandList);

	commandList.EndRenderPass();
	commandList.EndDebugMarker();
}

void KDeferredRenderer::PostProcessResult(KRHICommandList& commandList)
{
	commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	commandList.BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_POSTPROCESS_RESULT].debugMarker, glm::vec4(1));
	commandList.BeginRenderPass(m_RenderPass[DRS_STATE_POSTPROCESS_RESULT], SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_RenderPass[DRS_STATE_POSTPROCESS_RESULT]->GetViewPort());

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = m_DrawProcessResultPipeline;
	command.pipeline->GetHandle(m_RenderPass[DRS_STATE_POSTPROCESS_RESULT], command.pipelineHandle);
	command.indexDraw = true;
	commandList.Render(command);

	commandList.EndRenderPass();
	commandList.EndDebugMarker();

	commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
}

void KDeferredRenderer::PrePass(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
}

void KDeferredRenderer::MainBasePass(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	ExecuteRenderPass(commandList, DRS_STAGE_MAIN_BASE_PASS, cullRes);
}

void KDeferredRenderer::PostBasePass(KRHICommandList& commandList)
{
	ExecuteRenderPass(commandList, DRS_STAGE_POST_BASE_PASS, {});
}

void KDeferredRenderer::ForwardOpaque(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	ExecuteRenderPass(commandList, DRS_STAGE_FORWARD_OPAQUE, cullRes);
}

void KDeferredRenderer::CopyOpaqueColor(KRHICommandList& commandList)
{
	{
		commandList.BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_COPY_OPAQUE_COLOR].debugMarker, glm::vec4(1));

		commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		commandList.BeginRenderPass(m_RenderPass[DRS_STATE_COPY_OPAQUE_COLOR], SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(m_RenderPass[DRS_STATE_COPY_OPAQUE_COLOR]->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.pipeline = m_DrawSceneColorPipeline;
		command.pipeline->GetHandle(m_RenderPass[DRS_STATE_COPY_OPAQUE_COLOR], command.pipelineHandle);
		command.indexDraw = true;
		commandList.Render(command);

		commandList.EndRenderPass();
		commandList.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		commandList.EndDebugMarker();
	}
}

void KDeferredRenderer::ForwardTransprant(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	if (KRenderGlobal::EnablePeeling)
	{
		KRenderGlobal::DepthPeeling.Execute(commandList, cullRes);
	}
	else
	{
		ExecuteRenderPass(commandList, DRS_STAGE_FORWARD_TRANSPRANT, cullRes);
	}
}

void KDeferredRenderer::DebugObject(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	commandList.BeginDebugMarker(GDeferredRenderStageDescription[DRS_STATE_DEBUG_OBJECT].debugMarker, glm::vec4(1));
	commandList.BeginRenderPass(m_RenderPass[DRS_STATE_DEBUG_OBJECT], SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_RenderPass[DRS_STATE_DEBUG_OBJECT]->GetViewPort());

	KRenderStageStatistics& statistics = m_Statistics[DRS_STATE_DEBUG_OBJECT];
	statistics.Reset();

	std::vector<KRenderCommand> commands;
	std::vector<glm::vec3> positions;
	std::vector<size_t> sortedIndices;

	auto BuildCommand = [&statistics, &commands, &sortedIndices, &positions, this](const std::vector<KMaterialSubMeshPtr>& materialSubMeshes, const KConstantDefinition::DEBUG& objectData)
	{
		glm::vec3 position = KMath::ExtractPosition(objectData.MODEL.MODEL);

		for (KMaterialSubMeshPtr materialSubMesh : materialSubMeshes)
		{
			KRenderCommand command;

			if (materialSubMesh->GetRenderCommand(RENDER_STAGE_DEBUG_TRIANGLE, command))
			{
				KDynamicConstantBufferUsage objectUsage;
				objectUsage.binding = SHADER_BINDING_OBJECT;
				objectUsage.range = sizeof(objectData);
				KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

				command.dynamicConstantUsages.push_back(objectUsage);

				++statistics.drawcalls;
				if (command.indexDraw)
				{
					statistics.faces += command.indexData->indexCount / 3;
					statistics.primtives += command.indexData->indexCount;
				}
				else
				{
					statistics.faces += command.vertexData->vertexCount / 3;
					statistics.primtives += command.vertexData->vertexCount;
				}

				command.pipeline->GetHandle(m_RenderPass[DRS_STATE_DEBUG_OBJECT], command.pipelineHandle);

				if (command.Complete())
				{
					sortedIndices.push_back(commands.size());
					positions.push_back(position);
					commands.push_back(std::move(command));
				}
			}

			if (materialSubMesh->GetRenderCommand(RENDER_STAGE_DEBUG_LINE, command))
			{
				KDynamicConstantBufferUsage objectUsage;
				objectUsage.binding = SHADER_BINDING_OBJECT;
				objectUsage.range = sizeof(objectData);
				KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

				command.dynamicConstantUsages.push_back(objectUsage);

				++statistics.drawcalls;
				if (command.indexDraw)
				{
					statistics.faces += command.indexData->indexCount / 2;
					statistics.primtives += command.indexData->indexCount;
				}
				else
				{
					statistics.faces += command.vertexData->vertexCount / 2;
					statistics.primtives += command.vertexData->vertexCount;
				}

				command.pipeline->GetHandle(m_RenderPass[DRS_STATE_DEBUG_OBJECT], command.pipelineHandle);

				if (command.Complete())
				{
					sortedIndices.push_back(commands.size());
					positions.push_back(position);
					commands.push_back(std::move(command));
				}
			}
		}
	};

	for (IKEntity* entity : cullRes)
	{
		KRenderComponent* render = nullptr;
		KTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_RENDER, &render) && entity->GetComponent(CT_TRANSFORM, &transform))
		{
			const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();

			KConstantDefinition::DEBUG objectData;
			objectData.MODEL = transform->FinalTransform();
			objectData.COLOR = render->GetUtilityColor();

			BuildCommand(materialSubMeshes, objectData);
		}

		KDebugComponent* debug = nullptr;
		if (entity->GetComponent(CT_DEBUG, &debug))
		{
			const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = debug->GetMaterialSubMeshs();
			const std::vector<glm::vec4>& colors = debug->GetColors();

			assert(colors.size() == materialSubMeshes.size());

			for (size_t i = 0; i < materialSubMeshes.size(); ++i)
			{
				KConstantDefinition::DEBUG objectData;
				objectData.MODEL.MODEL = glm::mat4(1.0f);
				objectData.MODEL.PRVE_MODEL = glm::mat4(1.0f);
				objectData.COLOR = colors[i];

				BuildCommand({ materialSubMeshes[i] }, objectData);
			}
		}
	}

	const glm::vec3& cameraPos = m_Camera->GetPosition();
	const glm::vec3& cameraForward = m_Camera->GetForward();

	std::sort(sortedIndices.begin(), sortedIndices.end(), [&cameraPos, &cameraForward, &positions](size_t lhs, size_t rhs) ->bool
	{
		const glm::vec3 lhsPos = positions[lhs];
		const glm::vec3 rhsPos = positions[rhs];

		const float lhsDis = glm::dot(lhsPos - cameraPos, cameraForward);
		const float rhsDis = glm::dot(rhsPos - cameraPos, cameraForward);

		return lhsDis > rhsDis;
	});

	for (size_t index : sortedIndices)
	{
		KRenderCommand& command = commands[index];
		IKPipelineHandlePtr handle = nullptr;
		if (command.pipeline->GetHandle(m_RenderPass[DRS_STATE_DEBUG_OBJECT], handle))
		{
			commandList.Render(command);
		}
	}

	std::for_each(m_RenderCallFuncs[DRS_STATE_DEBUG_OBJECT].begin(), m_RenderCallFuncs[DRS_STATE_DEBUG_OBJECT].end(), [this, &commandList](RenderPassCallFuncType* func)
	{
		(*func)(m_RenderPass[DRS_STATE_DEBUG_OBJECT], commandList);
	});

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(GDeferredRenderStageDescription[DRS_STATE_DEBUG_OBJECT].debugMarker, statistics);

	commandList.EndRenderPass();
	commandList.EndDebugMarker();
}

void KDeferredRenderer::DeferredLighting(KRHICommandList& commandList)
{
	commandList.BeginDebugMarker(GDeferredRenderStageDescription[DRS_STAGE_DEFERRED_LIGHTING].debugMarker, glm::vec4(1));
	commandList.BeginRenderPass(m_RenderPass[DRS_STAGE_DEFERRED_LIGHTING], SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_RenderPass[DRS_STAGE_DEFERRED_LIGHTING]->GetViewPort());

	IKRenderPassPtr renderPass = m_RenderPass[DRS_STAGE_DEFERRED_LIGHTING];

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = m_LightingPipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;

	struct Debug
	{
		uint32_t debugOption;
	} debug;

	debug.debugOption = m_DebugOption;

	KDynamicConstantBufferUsage debugUsage;	
	debugUsage.binding = SHADER_BINDING_DEBUG;
	debugUsage.range = sizeof(debug);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&debug, debugUsage);

	command.dynamicConstantUsages.push_back(debugUsage);

	commandList.Render(command);

	commandList.EndRenderPass();
	commandList.EndDebugMarker();
}

void KDeferredRenderer::DrawFinalResult(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	commandList.BeginDebugMarker("DrawFinalResult", glm::vec4(1));
	commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(renderPass->GetViewPort());

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = m_DrawFinalPipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;
	commandList.Render(command);

	commandList.EndRenderPass();
	commandList.EndDebugMarker();
}

void KDeferredRenderer::Reload()
{
	if (m_QuadVS)
		m_QuadVS->Reload();
	if (m_DeferredLightingFS)
		m_DeferredLightingFS->Reload();
	if (m_SceneColorDrawFS)
		m_SceneColorDrawFS->Reload();
	if (m_LightingPipeline)
		m_LightingPipeline->Reload(false);
	if (m_DrawSceneColorPipeline)
		m_DrawSceneColorPipeline->Reload(false);
	if (m_DrawProcessResultPipeline)
		m_DrawProcessResultPipeline->Reload(false);
	if (m_DrawFinalPipeline)
		m_DrawFinalPipeline->Reload(false);
}