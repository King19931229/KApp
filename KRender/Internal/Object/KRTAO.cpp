#include "KRTAO.h"
#include "Internal/KRenderGlobal.h"
#include "interface/IKComputePipeline.h"

KRTAO::KRTAO()
	: m_Camera(nullptr)
	, m_PrevCamMat(glm::mat4(1.0f))
	, m_Width(1024)
	, m_Height(1024)
	, m_Enable(true)
{
}

KRTAO::~KRTAO()
{
	ASSERT_RESULT(!m_AOComputePipeline);
	ASSERT_RESULT(!m_AOUniformBuffer);
}

void KRTAO::UpdateAOUniform()
{
	if (m_AOUniformBuffer)
	{
		constexpr size_t SIZE = sizeof(AoControl) - MEMBER_SIZE(AoControl, frame);

		glm::mat4 camMat = m_Camera->GetViewMatrix();
		if (memcmp(&m_PrevCamMat, &camMat, sizeof(camMat)) != 0 ||
			memcmp(&m_PrevParameters, &m_AOParameters, SIZE) != 0)
		{
			m_PrevCamMat = camMat;
			memcpy(&m_PrevParameters, &m_AOParameters, SIZE);
		}
		m_AOUniformBuffer->Write(&m_AOParameters);
		++m_AOParameters.frame;
	}
}

void KRTAO::UpdateMeanUniform()
{
	if (m_MeanUniformBuffer)
	{
		m_MeanUniformBuffer->Write(&m_MeanParameters);
	}
}

bool KRTAO::Init(IKRayTraceScene* scene)
{
	if (scene)
	{
		IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

		renderDevice->CreateRenderTarget(m_PrevAOTarget);
		renderDevice->CreateRenderTarget(m_CurAOTarget);

		renderDevice->CreateRenderTarget(m_PrevHitDistanceTarget);
		renderDevice->CreateRenderTarget(m_CurHitDistanceTarget);

		renderDevice->CreateRenderTarget(m_PrevNormalDepthTarget);
		renderDevice->CreateRenderTarget(m_CurNormalDepthTarget);

		renderDevice->CreateRenderTarget(m_PrevSquaredMeanTarget);
		renderDevice->CreateRenderTarget(m_CurSquaredMeanTarget);

		renderDevice->CreateRenderTarget(m_PrevTSPP);
		renderDevice->CreateRenderTarget(m_CurTSPP);

		renderDevice->CreateRenderTarget(m_MeanVarianceTarget[0]);
		renderDevice->CreateRenderTarget(m_MeanVarianceTarget[1]);

		renderDevice->CreateRenderTarget(m_ReprojectedTarget);
		renderDevice->CreateRenderTarget(m_VarianceTarget);
		renderDevice->CreateRenderTarget(m_BlurStrengthTarget);
		renderDevice->CreateRenderTarget(m_AtrousAOTarget);

		renderDevice->CreateRenderTarget(m_BlurTempTarget);

		Resize();

		m_Camera = scene->GetCamera();
		m_PrevCamMat = glm::mat4(0.0f);

		renderDevice->CreateUniformBuffer(m_AOUniformBuffer);
		m_AOUniformBuffer->InitMemory(sizeof(m_AOParameters), &m_AOParameters);
		m_AOUniformBuffer->InitDevice();

		renderDevice->CreateUniformBuffer(m_MeanUniformBuffer);
		m_MeanUniformBuffer->InitMemory(sizeof(m_MeanParameters), &m_MeanParameters);
		m_MeanUniformBuffer->InitDevice();

		IKRenderTargetPtr gbuffer0 = KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0);
		IKRenderTargetPtr gbuffer1 = KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET1);
		IKRenderTargetPtr ao = KRenderGlobal::GBuffer.GetAOTarget();

		IKUniformBufferPtr& cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);

		KRenderDeviceProperties* property = nullptr;
		renderDevice->QueryProperty(&property);

		if (property->raytraceSupport)
		{
			renderDevice->CreateComputePipeline(m_AOComputePipeline);
			m_AOComputePipeline->BindStorageImage(BINDING_GBUFFER_RT0, gbuffer0->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_AOComputePipeline->BindStorageImage(BINDING_GBUFFER_RT1, gbuffer1->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_AOComputePipeline->BindAccelerationStructure(BINDING_AS, scene->GetTopDownAS(), true);
			m_AOComputePipeline->BindUniformBuffer(BINDING_CAMERA, cameraBuffer);
			m_AOComputePipeline->BindUniformBuffer(BINDING_UNIFORM, m_AOUniformBuffer);

			m_AOComputePipeline->BindStorageImage(BINDING_CUR_AO, m_CurAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_AOComputePipeline->BindStorageImage(BINDING_CUR_HITDISTANCE, m_CurHitDistanceTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_AOComputePipeline->BindStorageImage(BINDING_LOCAL_MEAN_VARIANCE_OUTPUT, m_MeanVarianceTarget[1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_AOComputePipeline->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_CurNormalDepthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			
			m_AOComputePipeline->Init("ao/rtao.comp");

			renderDevice->CreateComputePipeline(m_MeanHorizontalComputePipeline);
			m_MeanHorizontalComputePipeline->BindStorageImage(0, m_MeanVarianceTarget[1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_MeanHorizontalComputePipeline->BindUniformBuffer(2, m_MeanUniformBuffer);
			m_MeanHorizontalComputePipeline->BindStorageImage(1, m_MeanVarianceTarget[0]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_MeanHorizontalComputePipeline->Init("ao/mean_h.comp");

			renderDevice->CreateComputePipeline(m_MeanVerticalComputePipeline);
			m_MeanVerticalComputePipeline->BindStorageImage(0, m_MeanVarianceTarget[0]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_MeanVerticalComputePipeline->BindUniformBuffer(2, m_MeanUniformBuffer);
			m_MeanVerticalComputePipeline->BindStorageImage(1, m_MeanVarianceTarget[1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_MeanVerticalComputePipeline->Init("ao/mean_v.comp");

			renderDevice->CreateComputePipeline(m_ReprojectPipeline);
			m_ReprojectPipeline->BindStorageImage(BINDING_GBUFFER_RT0, gbuffer0->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ReprojectPipeline->BindStorageImage(BINDING_GBUFFER_RT1, gbuffer1->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);

			m_ReprojectPipeline->BindStorageImage(BINDING_PREV_NORMAL_DEPTH, m_PrevNormalDepthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ReprojectPipeline->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_CurNormalDepthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);

			m_ReprojectPipeline->BindStorageImage(BINDING_PREV_AO, m_PrevAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ReprojectPipeline->BindStorageImage(BINDING_PREV_HITDISTANCE, m_PrevHitDistanceTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ReprojectPipeline->BindStorageImage(BINDING_PREV_SQARED_MEAN, m_PrevSquaredMeanTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ReprojectPipeline->BindStorageImage(BINDING_PREV_TSPP, m_PrevTSPP->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ReprojectPipeline->BindStorageImage(BINDING_CUR_TSPP, m_CurTSPP->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);

			m_ReprojectPipeline->BindStorageImage(BINDING_REPROJECTED, m_ReprojectedTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_ReprojectPipeline->BindUniformBuffer(BINDING_CAMERA, cameraBuffer);
			m_ReprojectPipeline->BindUniformBuffer(BINDING_UNIFORM, m_AOUniformBuffer);
			m_ReprojectPipeline->Init("ao/reproject.comp");

			renderDevice->CreateComputePipeline(m_AOTemporalPipeline);
			m_AOTemporalPipeline->BindStorageImage(BINDING_REPROJECTED, m_ReprojectedTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_CUR_HITDISTANCE, m_CurHitDistanceTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_CUR_SQARED_MEAN, m_CurSquaredMeanTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_CUR_TSPP, m_CurTSPP->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_CUR_AO, m_CurAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_LOCAL_MEAN_VARIANCE_OUTPUT, m_MeanVarianceTarget[1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_VARIANCE, m_VarianceTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_BLUR_STRENGTH, m_BlurStrengthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_AOTemporalPipeline->BindUniformBuffer(BINDING_UNIFORM, m_AOUniformBuffer);
			m_AOTemporalPipeline->Init("ao/rtao_temp.comp");

			renderDevice->CreateComputePipeline(m_AtrousComputePipeline);
			m_AtrousComputePipeline->BindStorageImage(BINDING_VARIANCE, m_VarianceTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_AtrousComputePipeline->BindStorageImage(BINDING_CUR_AO, m_CurAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_AtrousComputePipeline->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_CurNormalDepthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_AtrousComputePipeline->BindStorageImage(BINDING_ATROUS_AO, m_AtrousAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_AtrousComputePipeline->Init("ao/atrous.comp");

			const char* blurHorizontalVertexShader[3] = { "ao/disconnected_1_h.comp", "ao/disconnected_2_h.comp", "ao/disconnected_3_h.comp" };
			const char* blurVerticalVertexShader[3] = { "ao/disconnected_1_v.comp", "ao/disconnected_2_v.comp", "ao/disconnected_3_v.comp" };
			for (uint32_t i = 0; i < 3; ++i)
			{
				renderDevice->CreateComputePipeline(m_BlurHorizontalComputePipeline[i]);
				renderDevice->CreateComputePipeline(m_BlurVerticalComputePipeline[i]);

				m_BlurHorizontalComputePipeline[i]->BindStorageImage(0, m_CurAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_BlurHorizontalComputePipeline[i]->BindStorageImage(1, m_BlurStrengthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_BlurHorizontalComputePipeline[i]->BindStorageImage(2, m_CurNormalDepthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_BlurHorizontalComputePipeline[i]->BindStorageImage(3, m_BlurTempTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_BlurHorizontalComputePipeline[i]->Init(blurHorizontalVertexShader[i]);

				m_BlurVerticalComputePipeline[i]->BindStorageImage(0, m_BlurTempTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_BlurVerticalComputePipeline[i]->BindStorageImage(1, m_BlurStrengthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_BlurVerticalComputePipeline[i]->BindStorageImage(2, m_CurNormalDepthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_BlurVerticalComputePipeline[i]->BindStorageImage(3, m_CurAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_BlurVerticalComputePipeline[i]->Init(blurVerticalVertexShader[i]);
			}

			renderDevice->CreateComputePipeline(m_ComposePipeline);
			m_ComposePipeline->BindStorageImage(BINDING_CUR_AO, m_CurAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_CUR_HITDISTANCE, m_CurHitDistanceTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_GBUFFER_RT0, gbuffer0->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_GBUFFER_RT1, gbuffer1->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_PREV_NORMAL_DEPTH, m_PrevNormalDepthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_CurNormalDepthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_REPROJECTED, m_ReprojectedTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
			m_ComposePipeline->BindUniformBuffer(BINDING_CAMERA, cameraBuffer);
			m_ComposePipeline->BindStorageImage(BINDING_COMPOSED, ao->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
			m_ComposePipeline->BindUniformBuffer(BINDING_UNIFORM, m_AOUniformBuffer);
			m_ComposePipeline->Init("ao/compose.comp");
		}
	}

	m_DebugDrawer.Init(KRenderGlobal::GBuffer.GetAOTarget() , 0, 0, 1, 1);
	return true;
}

bool KRTAO::UnInit()
{
	m_DebugDrawer.UnInit();

	SAFE_UNINIT(m_AOComputePipeline);
	SAFE_UNINIT(m_AOTemporalPipeline);
	SAFE_UNINIT(m_ComposePipeline);
	SAFE_UNINIT(m_MeanHorizontalComputePipeline);
	SAFE_UNINIT(m_MeanVerticalComputePipeline);
	SAFE_UNINIT(m_ReprojectPipeline);
	SAFE_UNINIT(m_AtrousComputePipeline);

	for (uint32_t i = 0; i < 3; ++i)
	{
		SAFE_UNINIT(m_BlurHorizontalComputePipeline[i]);
		SAFE_UNINIT(m_BlurVerticalComputePipeline[i]);
	}

	SAFE_UNINIT(m_PrevAOTarget);
	SAFE_UNINIT(m_CurAOTarget);

	SAFE_UNINIT(m_PrevHitDistanceTarget);
	SAFE_UNINIT(m_CurHitDistanceTarget);

	SAFE_UNINIT(m_PrevNormalDepthTarget);
	SAFE_UNINIT(m_CurNormalDepthTarget);

	SAFE_UNINIT(m_PrevSquaredMeanTarget);
	SAFE_UNINIT(m_CurSquaredMeanTarget);

	SAFE_UNINIT(m_PrevTSPP);
	SAFE_UNINIT(m_CurTSPP);

	SAFE_UNINIT(m_MeanVarianceTarget[0]);
	SAFE_UNINIT(m_MeanVarianceTarget[1]);

	SAFE_UNINIT(m_ReprojectedTarget);
	SAFE_UNINIT(m_VarianceTarget);
	SAFE_UNINIT(m_BlurStrengthTarget);
	SAFE_UNINIT(m_AtrousAOTarget);

	SAFE_UNINIT(m_BlurTempTarget);

	SAFE_UNINIT(m_AOUniformBuffer);
	SAFE_UNINIT(m_MeanUniformBuffer);

	m_Camera = nullptr;
	return true;
}

bool KRTAO::EnableDebugDraw()
{
	m_DebugDrawer.EnableDraw();
	return true;
}

bool KRTAO::DisableDebugDraw()
{
	m_DebugDrawer.DisableDraw();
	return true;
}

bool KRTAO::DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	return m_DebugDrawer.Render(renderPass, primaryBuffer);
}

bool KRTAO::Execute(IKCommandBufferPtr primaryBuffer)
{
	if (m_AOComputePipeline && m_Enable)
	{
		primaryBuffer->BeginDebugMarker("RTAO", glm::vec4(0, 1, 0, 0));

		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), IMAGE_LAYOUT_GENERAL);
		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET1)->GetFrameBuffer(), IMAGE_LAYOUT_GENERAL);
		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetAOTarget()->GetFrameBuffer(), IMAGE_LAYOUT_GENERAL);

		uint32_t groupX = (m_Width + (RTAO_GROUP_SIZE - 1)) / RTAO_GROUP_SIZE;
		uint32_t groupY = (m_Height + (RTAO_GROUP_SIZE - 1)) / RTAO_GROUP_SIZE;

		{
			primaryBuffer->BeginDebugMarker("RTAO_ComputeAO", glm::vec4(0, 1, 0, 0));
			UpdateAOUniform();
			m_AOComputePipeline->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_ComputeMeanVariance", glm::vec4(0, 1, 0, 0));
			UpdateMeanUniform();
			m_MeanHorizontalComputePipeline->Execute(primaryBuffer, groupX, groupY, 1);
			m_MeanVerticalComputePipeline->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_Reproject", glm::vec4(0, 1, 0, 0));
			m_ReprojectPipeline->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_Temporal", glm::vec4(0, 1, 0, 0));
			m_AOTemporalPipeline->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_Atrous", glm::vec4(0, 1, 0, 0));
			m_AtrousComputePipeline->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->Blit(m_AtrousAOTarget->GetFrameBuffer(), m_CurAOTarget->GetFrameBuffer());
			primaryBuffer->EndDebugMarker();
		}

		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				std::string marker = "RTAO_Blur_" + std::to_string(i);
				primaryBuffer->BeginDebugMarker(marker, glm::vec4(0, 1, 0, 0));
				m_BlurHorizontalComputePipeline[i]->Execute(primaryBuffer, groupX, groupY, 1);
				m_BlurVerticalComputePipeline[i]->Execute(primaryBuffer, groupX, groupY, 1);
				primaryBuffer->EndDebugMarker();
			}
		}

		{
			IKRenderTargetPtr aoTarget = KRenderGlobal::GBuffer.GetAOTarget();
			groupX = (aoTarget->GetFrameBuffer()->GetWidth() + (RTAO_GROUP_SIZE - 1)) / RTAO_GROUP_SIZE;
			groupY = (aoTarget->GetFrameBuffer()->GetHeight() + (RTAO_GROUP_SIZE - 1)) / RTAO_GROUP_SIZE;
			primaryBuffer->BeginDebugMarker("RTAO_Composed", glm::vec4(0, 1, 0, 0));
			m_ComposePipeline->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_BlitToPrevious", glm::vec4(0, 1, 0, 0));
			primaryBuffer->Blit(m_CurAOTarget->GetFrameBuffer(), m_PrevAOTarget->GetFrameBuffer());
			primaryBuffer->Blit(m_CurHitDistanceTarget->GetFrameBuffer(), m_PrevHitDistanceTarget->GetFrameBuffer());
			primaryBuffer->Blit(m_CurNormalDepthTarget->GetFrameBuffer(), m_PrevNormalDepthTarget->GetFrameBuffer());
			primaryBuffer->Blit(m_CurSquaredMeanTarget->GetFrameBuffer(), m_PrevSquaredMeanTarget->GetFrameBuffer());
			primaryBuffer->Blit(m_CurTSPP->GetFrameBuffer(), m_PrevTSPP->GetFrameBuffer());
			primaryBuffer->EndDebugMarker();
		}

		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET1)->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY);
		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetAOTarget()->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY);

		primaryBuffer->EndDebugMarker();
	}
	return true;
}

bool KRTAO::ReloadShader()
{
	if (m_ComposePipeline)
	{
		m_AOComputePipeline->Reload();
		m_AOTemporalPipeline->Reload();
		m_MeanHorizontalComputePipeline->Reload();
		m_MeanVerticalComputePipeline->Reload();
		m_ReprojectPipeline->Reload();
		m_AtrousComputePipeline->Reload();
		m_ComposePipeline->Reload();
		return true;
	}
	return false;
}

void KRTAO::Resize()
{
	if (m_AtrousAOTarget)
	{
		IKSwapChain* chain = KRenderGlobal::RenderDevice->GetSwapChain();

		uint32_t newWidth = m_Width;
		uint32_t newHeight = m_Height;

		if (chain->GetWidth() && chain->GetHeight())
		{
			newWidth = chain->GetWidth() / 2;
			newHeight = chain->GetHeight() / 2;
		}

		if (m_Width != newWidth || m_Height != newHeight)
		{
			m_PrevCamMat = glm::mat4(0.0f);
			m_Width = newWidth;
			m_Height = newHeight;
		}

		m_PrevAOTarget->UnInit();
		m_PrevAOTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);
		m_CurAOTarget->UnInit();
		m_CurAOTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

		m_PrevHitDistanceTarget->UnInit();
		m_PrevHitDistanceTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);
		m_CurHitDistanceTarget->UnInit();
		m_CurHitDistanceTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

		m_PrevNormalDepthTarget->UnInit();
		m_PrevNormalDepthTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16G16B16A16_FLOAT);
		m_CurNormalDepthTarget->UnInit();
		m_CurNormalDepthTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16G16B16A16_FLOAT);

		m_PrevSquaredMeanTarget->UnInit();
		m_PrevSquaredMeanTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);
		m_CurSquaredMeanTarget->UnInit();
		m_CurSquaredMeanTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

		m_PrevTSPP->UnInit();
		m_PrevTSPP->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);
		m_CurTSPP->UnInit();
		m_CurTSPP->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

		m_MeanVarianceTarget[0]->UnInit();
		m_MeanVarianceTarget[0]->InitFromStorage(m_Width, m_Height, 1, EF_R16G16_FLOAT);
		m_MeanVarianceTarget[1]->UnInit();
		m_MeanVarianceTarget[1]->InitFromStorage(m_Width, m_Height, 1, EF_R16G16_FLOAT);

		m_ReprojectedTarget->UnInit();
		m_ReprojectedTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16G16B16A16_FLOAT);

		m_VarianceTarget->UnInit();
		m_VarianceTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

		m_BlurStrengthTarget->UnInit();
		m_BlurStrengthTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

		m_AtrousAOTarget->UnInit();
		m_AtrousAOTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

		m_BlurTempTarget->UnInit();
		m_BlurTempTarget->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);
	}
}