#include "KRTAO.h"
#include "Internal/KRenderGlobal.h"
#include "interface/IKComputePipeline.h"

KRTAO::KRTAO()
	: m_Camera(nullptr)
	, m_PrevCamMat(glm::mat4(1.0f))
	, m_Width(1024)
	, m_Height(1024)
	, m_CurrentAOIndex(0)
	, m_Enable(true)
{
	ZERO_MEMORY(m_AOComputePipeline);
	ZERO_MEMORY(m_ReprojectPipeline);
	ZERO_MEMORY(m_AOTemporalPipeline);
	ZERO_MEMORY(m_AtrousComputePipeline);
	ZERO_MEMORY(m_ComposePipeline);
	ZERO_MEMORY(m_BlurHorizontalComputePipeline);
	ZERO_MEMORY(m_BlurVerticalComputePipeline);
}

KRTAO::~KRTAO()
{
	ASSERT_RESULT(!m_AOComputePipeline[0]);
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

		KRenderDeviceProperties* property = nullptr;
		renderDevice->QueryProperty(&property);

		if (property->raytraceSupport)
		{
			for (uint32_t i = 0; i < 2; ++i)
			{
				renderDevice->CreateRenderTarget(m_AOTarget[i]);
				renderDevice->CreateRenderTarget(m_HitDistanceTarget[i]);
				renderDevice->CreateRenderTarget(m_NormalDepthTarget[i]);
				renderDevice->CreateRenderTarget(m_SquaredMeanTarget[i]);
				renderDevice->CreateRenderTarget(m_TSPP[i]);
			}

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

			for (uint32_t i = 0; i < 2; ++i)
			{
				renderDevice->CreateComputePipeline(m_AOComputePipeline[i]);
				m_AOComputePipeline[i]->BindStorageImage(BINDING_GBUFFER_RT0, gbuffer0->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_AOComputePipeline[i]->BindStorageImage(BINDING_GBUFFER_RT1, gbuffer1->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_AOComputePipeline[i]->BindAccelerationStructure(BINDING_AS, scene->GetTopDownAS(), true);
				m_AOComputePipeline[i]->BindUniformBuffer(BINDING_CAMERA, cameraBuffer);
				m_AOComputePipeline[i]->BindUniformBuffer(BINDING_UNIFORM, m_AOUniformBuffer);

				m_AOComputePipeline[i]->BindStorageImage(BINDING_CUR_AO, m_AOTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_AOComputePipeline[i]->BindStorageImage(BINDING_CUR_HITDISTANCE, m_HitDistanceTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_AOComputePipeline[i]->BindStorageImage(BINDING_LOCAL_MEAN_VARIANCE_OUTPUT, m_MeanVarianceTarget[1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_AOComputePipeline[i]->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_NormalDepthTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);

				m_AOComputePipeline[i]->Init("ao/rtao.comp");
			}

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

			for (uint32_t i = 0; i < 2; ++i)
			{
				renderDevice->CreateComputePipeline(m_ReprojectPipeline[i]);
				m_ReprojectPipeline[i]->BindStorageImage(BINDING_GBUFFER_RT0, gbuffer0->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ReprojectPipeline[i]->BindStorageImage(BINDING_GBUFFER_RT1, gbuffer1->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);

				m_ReprojectPipeline[i]->BindStorageImage(BINDING_PREV_NORMAL_DEPTH, m_NormalDepthTarget[(i + 1) & 1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ReprojectPipeline[i]->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_NormalDepthTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);

				m_ReprojectPipeline[i]->BindStorageImage(BINDING_PREV_AO, m_AOTarget[(i + 1) & 1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ReprojectPipeline[i]->BindStorageImage(BINDING_PREV_HITDISTANCE, m_HitDistanceTarget[(i + 1) & 1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ReprojectPipeline[i]->BindStorageImage(BINDING_PREV_SQARED_MEAN, m_SquaredMeanTarget[(i + 1) & 1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ReprojectPipeline[i]->BindStorageImage(BINDING_PREV_TSPP, m_TSPP[(i + 1) & 1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ReprojectPipeline[i]->BindStorageImage(BINDING_CUR_TSPP, m_TSPP[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);

				m_ReprojectPipeline[i]->BindStorageImage(BINDING_REPROJECTED, m_ReprojectedTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_ReprojectPipeline[i]->BindUniformBuffer(BINDING_CAMERA, cameraBuffer);
				m_ReprojectPipeline[i]->BindUniformBuffer(BINDING_UNIFORM, m_AOUniformBuffer);
				m_ReprojectPipeline[i]->Init("ao/reproject.comp");
			}

			for (uint32_t i = 0; i < 2; ++i)
			{
				renderDevice->CreateComputePipeline(m_AOTemporalPipeline[i]);
				m_AOTemporalPipeline[i]->BindStorageImage(BINDING_REPROJECTED, m_ReprojectedTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_AOTemporalPipeline[i]->BindStorageImage(BINDING_CUR_HITDISTANCE, m_HitDistanceTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);
				m_AOTemporalPipeline[i]->BindStorageImage(BINDING_CUR_SQARED_MEAN, m_SquaredMeanTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);
				m_AOTemporalPipeline[i]->BindStorageImage(BINDING_CUR_TSPP, m_TSPP[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);
				m_AOTemporalPipeline[i]->BindStorageImage(BINDING_CUR_AO, m_AOTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, true);
				m_AOTemporalPipeline[i]->BindStorageImage(BINDING_LOCAL_MEAN_VARIANCE_OUTPUT, m_MeanVarianceTarget[1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_AOTemporalPipeline[i]->BindStorageImage(BINDING_VARIANCE, m_VarianceTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_AOTemporalPipeline[i]->BindStorageImage(BINDING_BLUR_STRENGTH, m_BlurStrengthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_AOTemporalPipeline[i]->BindUniformBuffer(BINDING_UNIFORM, m_AOUniformBuffer);
				m_AOTemporalPipeline[i]->Init("ao/rtao_temp.comp");
			}

			for (uint32_t i = 0; i < 2; ++i)
			{
				renderDevice->CreateComputePipeline(m_AtrousComputePipeline[i]);
				m_AtrousComputePipeline[i]->BindStorageImage(BINDING_VARIANCE, m_VarianceTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_AtrousComputePipeline[i]->BindStorageImage(BINDING_CUR_AO, m_AOTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_AtrousComputePipeline[i]->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_NormalDepthTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_AtrousComputePipeline[i]->BindStorageImage(BINDING_ATROUS_AO, m_AtrousAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_AtrousComputePipeline[i]->Init("ao/atrous.comp");
			}

			const char* blurHorizontalVertexShader[3] = { "ao/disconnected_1_h.comp", "ao/disconnected_2_h.comp", "ao/disconnected_3_h.comp" };
			const char* blurVerticalVertexShader[3] = { "ao/disconnected_1_v.comp", "ao/disconnected_2_v.comp", "ao/disconnected_3_v.comp" };

			for (uint32_t i = 0; i < 2; ++i)
			{
				for (uint32_t j = 0; j < 3; ++j)
				{
					renderDevice->CreateComputePipeline(m_BlurHorizontalComputePipeline[i][j]);
					renderDevice->CreateComputePipeline(m_BlurVerticalComputePipeline[i][j]);

					m_BlurHorizontalComputePipeline[i][j]->BindStorageImage(0, m_AtrousAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
					m_BlurHorizontalComputePipeline[i][j]->BindStorageImage(1, m_BlurStrengthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
					m_BlurHorizontalComputePipeline[i][j]->BindStorageImage(2, m_NormalDepthTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
					m_BlurHorizontalComputePipeline[i][j]->BindStorageImage(3, m_BlurTempTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
					m_BlurHorizontalComputePipeline[i][j]->Init(blurHorizontalVertexShader[j]);

					m_BlurVerticalComputePipeline[i][j]->BindStorageImage(0, m_BlurTempTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
					m_BlurVerticalComputePipeline[i][j]->BindStorageImage(1, m_BlurStrengthTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
					m_BlurVerticalComputePipeline[i][j]->BindStorageImage(2, m_NormalDepthTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
					m_BlurVerticalComputePipeline[i][j]->BindStorageImage(3, m_AtrousAOTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
					m_BlurVerticalComputePipeline[i][j]->Init(blurVerticalVertexShader[j]);
				}
			}

			for (uint32_t i = 0; i < 2; ++i)
			{
				renderDevice->CreateComputePipeline(m_ComposePipeline[i]);
				m_ComposePipeline[i]->BindStorageImage(BINDING_CUR_AO, m_AOTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ComposePipeline[i]->BindStorageImage(BINDING_CUR_HITDISTANCE, m_HitDistanceTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ComposePipeline[i]->BindStorageImage(BINDING_GBUFFER_RT0, gbuffer0->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ComposePipeline[i]->BindStorageImage(BINDING_GBUFFER_RT1, gbuffer1->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ComposePipeline[i]->BindStorageImage(BINDING_PREV_NORMAL_DEPTH, m_NormalDepthTarget[(i + 1) & 1]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ComposePipeline[i]->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_NormalDepthTarget[i]->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ComposePipeline[i]->BindStorageImage(BINDING_REPROJECTED, m_ReprojectedTarget->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
				m_ComposePipeline[i]->BindUniformBuffer(BINDING_CAMERA, cameraBuffer);
				m_ComposePipeline[i]->BindStorageImage(BINDING_COMPOSED, ao->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
				m_ComposePipeline[i]->BindUniformBuffer(BINDING_UNIFORM, m_AOUniformBuffer);
				m_ComposePipeline[i]->Init("ao/compose.comp");
			}
		}
	}

	m_DebugDrawer.Init(KRenderGlobal::GBuffer.GetAOTarget(), 0, 0, 1, 1);

	m_CurrentAOIndex = 0;

	return true;
}

bool KRTAO::UnInit()
{
	m_DebugDrawer.UnInit();

	for (uint32_t i = 0; i < 2; ++i)
	{
		SAFE_UNINIT(m_AOComputePipeline[i]);
		SAFE_UNINIT(m_AOTemporalPipeline[i]);
		SAFE_UNINIT(m_ComposePipeline[i]);
		SAFE_UNINIT(m_AtrousComputePipeline[i]);
		SAFE_UNINIT(m_ReprojectPipeline[i]);

		for (uint32_t j = 0; j < 3; ++j)
		{
			SAFE_UNINIT(m_BlurHorizontalComputePipeline[i][j]);
			SAFE_UNINIT(m_BlurVerticalComputePipeline[i][j]);
		}
	}

	SAFE_UNINIT(m_MeanHorizontalComputePipeline);
	SAFE_UNINIT(m_MeanVerticalComputePipeline);

	for (uint32_t i = 0; i < 2; ++i)
	{
		SAFE_UNINIT(m_AOTarget[i]);
		SAFE_UNINIT(m_HitDistanceTarget[i]);
		SAFE_UNINIT(m_NormalDepthTarget[i]);
		SAFE_UNINIT(m_SquaredMeanTarget[i]);
		SAFE_UNINIT(m_TSPP[i]);
	}

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

bool KRTAO::Execute(IKCommandBufferPtr primaryBuffer, IKQueuePtr graphicsQueue, IKQueuePtr computeQueue)
{
	if (m_AOComputePipeline[m_CurrentAOIndex] && m_Enable)
	{
		primaryBuffer->BeginDebugMarker("RTAO", glm::vec4(1));

		uint32_t groupX = (m_Width + (RTAO_GROUP_SIZE - 1)) / RTAO_GROUP_SIZE;
		uint32_t groupY = (m_Height + (RTAO_GROUP_SIZE - 1)) / RTAO_GROUP_SIZE;

		{
			primaryBuffer->BeginDebugMarker("RTAO_ComputeAO", glm::vec4(1));
			UpdateAOUniform();
			m_AOComputePipeline[m_CurrentAOIndex]->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_ComputeMeanVariance", glm::vec4(1));
			UpdateMeanUniform();
			m_MeanHorizontalComputePipeline->Execute(primaryBuffer, groupX, groupY, 1);
			m_MeanVerticalComputePipeline->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_Reproject", glm::vec4(1));
			m_ReprojectPipeline[m_CurrentAOIndex]->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_Temporal", glm::vec4(1));
			m_AOTemporalPipeline[m_CurrentAOIndex]->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("RTAO_Atrous", glm::vec4(1));
			m_AtrousComputePipeline[m_CurrentAOIndex]->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				std::string marker = "RTAO_Blur_" + std::to_string(i);
				primaryBuffer->BeginDebugMarker(marker, glm::vec4(1));
				m_BlurHorizontalComputePipeline[m_CurrentAOIndex][i]->Execute(primaryBuffer, groupX, groupY, 1);
				m_BlurVerticalComputePipeline[m_CurrentAOIndex][i]->Execute(primaryBuffer, groupX, groupY, 1);
				primaryBuffer->EndDebugMarker();
			}
		}

		{
			IKRenderTargetPtr aoTarget = KRenderGlobal::GBuffer.GetAOTarget();
			groupX = (aoTarget->GetFrameBuffer()->GetWidth() + (RTAO_GROUP_SIZE - 1)) / RTAO_GROUP_SIZE;
			groupY = (aoTarget->GetFrameBuffer()->GetHeight() + (RTAO_GROUP_SIZE - 1)) / RTAO_GROUP_SIZE;
			primaryBuffer->BeginDebugMarker("RTAO_Composed", glm::vec4(1));
			m_ComposePipeline[m_CurrentAOIndex]->Execute(primaryBuffer, groupX, groupY, 1);
			primaryBuffer->EndDebugMarker();
		}

		primaryBuffer->EndDebugMarker();

		m_CurrentAOIndex = (m_CurrentAOIndex + 1) & 1;
	}
	return true;
}

bool KRTAO::ReloadShader()
{
	if (m_ComposePipeline)
	{
		for (uint32_t i = 0; i < 2; ++i)
		{
			m_AOComputePipeline[i]->Reload();
			m_AOTemporalPipeline[i]->Reload();
			m_ReprojectPipeline[i]->Reload();
			m_AtrousComputePipeline[i]->Reload();
			m_ComposePipeline[i]->Reload();
		}

		m_MeanHorizontalComputePipeline->Reload();
		m_MeanVerticalComputePipeline->Reload();

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

		for (uint32_t i = 0; i < 2; ++i)
		{
			m_AOTarget[i]->UnInit();
			m_AOTarget[i]->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

			m_HitDistanceTarget[i]->UnInit();
			m_HitDistanceTarget[i]->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

			m_NormalDepthTarget[i]->UnInit();
			m_NormalDepthTarget[i]->InitFromStorage(m_Width, m_Height, 1, EF_R16G16B16A16_FLOAT);

			m_SquaredMeanTarget[i]->UnInit();
			m_SquaredMeanTarget[i]->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);

			m_TSPP[i]->UnInit();
			m_TSPP[i]->InitFromStorage(m_Width, m_Height, 1, EF_R16_FLOAT);
		}

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