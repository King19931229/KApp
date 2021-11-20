#include "KRTAO.h"
#include "Internal/KRenderGlobal.h"
#include "interface/IKComputePipeline.h"

KRTAO::KRTAO()
	: m_Camera(nullptr)
	, m_PrevCamMat(glm::mat4(1.0f))
	, m_Width(1024)
	, m_Height(1024)
{
}

KRTAO::~KRTAO()
{
	ASSERT_RESULT(!m_AOComputePipeline);
	ASSERT_RESULT(!m_RenderTarget[0]);
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

		renderDevice->CreateRenderTarget(m_RenderTarget[0]);
		renderDevice->CreateRenderTarget(m_RenderTarget[1]);
		renderDevice->CreateRenderTarget(m_MeanVarianceTarget[0]);
		renderDevice->CreateRenderTarget(m_MeanVarianceTarget[1]);
		renderDevice->CreateRenderTarget(m_NormalDepthTarget[0]);
		renderDevice->CreateRenderTarget(m_NormalDepthTarget[1]);
		renderDevice->CreateRenderTarget(m_CurrentTarget);
		renderDevice->CreateRenderTarget(m_TemporalMeanSqaredMean);
		renderDevice->CreateRenderTarget(m_AtrousTarget);
		renderDevice->CreateRenderTarget(m_ComposedTarget);

		UpdateSize();

		m_Camera = scene->GetCamera();
		m_PrevCamMat = glm::mat4(0.0f);

		renderDevice->CreateUniformBuffer(m_AOUniformBuffer);
		m_AOUniformBuffer->InitMemory(sizeof(m_AOParameters), &m_AOParameters);
		m_AOUniformBuffer->InitDevice();

		renderDevice->CreateUniformBuffer(m_MeanUniformBuffer);
		m_MeanUniformBuffer->InitMemory(sizeof(m_MeanParameters), &m_MeanParameters);
		m_MeanUniformBuffer->InitDevice();

		IKRayTracePipeline* rayPipeline = scene->GetRayTracePipeline();

		IKRenderTargetPtr normalBuffer = KRenderGlobal::GBuffer.GetGBufferTarget0();
		IKRenderTargetPtr positionBuffer = KRenderGlobal::GBuffer.GetGBufferTarget1();
		IKRenderTargetPtr velocityBuffer = KRenderGlobal::GBuffer.GetGBufferTarget2();

		KRenderDeviceProperties* property = nullptr;
		renderDevice->QueryProperty(&property);

		if (property->raytraceSupport)
		{
			renderDevice->CreateComputePipeline(m_AOComputePipeline);
			m_AOComputePipeline->BindStorageImage(BINDING_GBUFFER_NORMAL, normalBuffer->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOComputePipeline->BindStorageImage(BINDING_GBUFFER_POSITION, positionBuffer->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOComputePipeline->BindAccelerationStructure(BINDING_AS, rayPipeline->GetTopdownAS(), true);
			m_AOComputePipeline->BindUniformBuffer(BDINING_UNIFORM, m_AOUniformBuffer);
			m_AOComputePipeline->BindStorageImage(BINDING_CUR, m_CurrentTarget->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_AOComputePipeline->BindStorageImage(BINDING_LOCAL_MEAN_VARIANCE_OUTPUT, m_MeanVarianceTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_AOComputePipeline->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_NormalDepthTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_AOComputePipeline->Init("ao/rtao.comp");

			renderDevice->CreateComputePipeline(m_MeanHorizontalComputePipeline);
			m_MeanHorizontalComputePipeline->BindStorageImage(0, m_MeanVarianceTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_MeanHorizontalComputePipeline->BindUniformBuffer(2, m_MeanUniformBuffer);
			m_MeanHorizontalComputePipeline->BindStorageImage(1, m_MeanVarianceTarget[0]->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_MeanHorizontalComputePipeline->Init("ao/mean_h.comp");

			renderDevice->CreateComputePipeline(m_MeanVerticalComputePipeline);
			m_MeanVerticalComputePipeline->BindStorageImage(0, m_MeanVarianceTarget[0]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_MeanVerticalComputePipeline->BindUniformBuffer(2, m_MeanUniformBuffer);
			m_MeanVerticalComputePipeline->BindStorageImage(1, m_MeanVarianceTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_MeanVerticalComputePipeline->Init("ao/mean_v.comp");

			renderDevice->CreateComputePipeline(m_AOTemporalPipeline);
			m_AOTemporalPipeline->BindStorageImage(BINDING_GBUFFER_NORMAL, normalBuffer->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_GBUFFER_POSITION, positionBuffer->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_VELOCITY, velocityBuffer->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOTemporalPipeline->BindAccelerationStructure(BINDING_AS, rayPipeline->GetTopdownAS(), true);
			m_AOTemporalPipeline->BindUniformBuffer(BDINING_UNIFORM, m_AOUniformBuffer);
			m_AOTemporalPipeline->BindStorageImage(BINDING_LOCAL_MEAN_VARIANCE_INPUT, m_MeanVarianceTarget[0]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_PREV, m_RenderTarget[0]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_PREV_NORMAL_DEPTH, m_NormalDepthTarget[0]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_CUR_NORMAL_DEPTH, m_NormalDepthTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_CUR, m_CurrentTarget->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);

			m_AOTemporalPipeline->BindStorageImage(BINDING_TEMPORAL_SQAREDMEAN_VARIANCE, m_TemporalMeanSqaredMean->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_AOTemporalPipeline->BindStorageImage(BINDING_FINAL, m_RenderTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_AOTemporalPipeline->Init("ao/rtao_temp.comp");

			renderDevice->CreateComputePipeline(m_AtrousComputePipeline);
			m_AtrousComputePipeline->BindStorageImage(BINDING_GBUFFER_NORMAL, normalBuffer->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AtrousComputePipeline->BindStorageImage(BINDING_GBUFFER_POSITION, positionBuffer->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AtrousComputePipeline->BindStorageImage(BINDING_TEMPORAL_SQAREDMEAN_VARIANCE, m_TemporalMeanSqaredMean->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AtrousComputePipeline->BindStorageImage(BINDING_FINAL, m_RenderTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AtrousComputePipeline->BindStorageImage(BINDING_CUR, m_CurrentTarget->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_AtrousComputePipeline->BindStorageImage(BINDING_ATROUS, m_AtrousTarget->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_AtrousComputePipeline->Init("ao/atrous.comp");

			renderDevice->CreateComputePipeline(m_ComposePipeline);
			m_ComposePipeline->BindStorageImage(BINDING_LOCAL_MEAN_VARIANCE_INPUT, m_MeanVarianceTarget[0]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_LOCAL_MEAN_VARIANCE_OUTPUT, m_MeanVarianceTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_TEMPORAL_SQAREDMEAN_VARIANCE, m_TemporalMeanSqaredMean->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_PREV, m_RenderTarget[0]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_FINAL, m_RenderTarget[1]->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_CUR, m_CurrentTarget->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_ATROUS, m_AtrousTarget->GetFrameBuffer(), COMPUTE_IMAGE_IN, 0, true);
			m_ComposePipeline->BindStorageImage(BINDING_COMPOSED, m_ComposedTarget->GetFrameBuffer(), COMPUTE_IMAGE_OUT, 0, true);
			m_ComposePipeline->Init("ao/compose.comp");
		}
	}

	m_DebugDrawer.Init(m_ComposedTarget);
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
	SAFE_UNINIT(m_AtrousComputePipeline);
	SAFE_UNINIT(m_RenderTarget[0]);
	SAFE_UNINIT(m_RenderTarget[1]);
	SAFE_UNINIT(m_MeanVarianceTarget[0]);
	SAFE_UNINIT(m_MeanVarianceTarget[1]);
	SAFE_UNINIT(m_NormalDepthTarget[0]);
	SAFE_UNINIT(m_NormalDepthTarget[1]);
	SAFE_UNINIT(m_CurrentTarget);
	SAFE_UNINIT(m_ComposedTarget);
	SAFE_UNINIT(m_AtrousTarget);
	SAFE_UNINIT(m_TemporalMeanSqaredMean);
	SAFE_UNINIT(m_AOUniformBuffer);
	SAFE_UNINIT(m_MeanUniformBuffer);
	m_Camera = nullptr;
	return true;
}

bool KRTAO::EnableDebugDraw(float x, float y, float width, float height)
{
	m_DebugDrawer.EnableDraw(x, y, width, height);
	return true;
}

bool KRTAO::DisableDebugDraw()
{
	m_DebugDrawer.DisableDraw();
	return true;
}

bool KRTAO::GetDebugRenderCommand(KRenderCommandList& commands)
{
	return m_DebugDrawer.GetDebugRenderCommand(commands);
}

bool KRTAO::Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (m_AOComputePipeline)
	{
		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetGBufferTarget0()->GetFrameBuffer(), IMAGE_LAYOUT_GENERAL);
		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetGBufferTarget1()->GetFrameBuffer(), IMAGE_LAYOUT_GENERAL);
		primaryBuffer->Translate(KRenderGlobal::GBuffer.GetGBufferTarget2()->GetFrameBuffer(), IMAGE_LAYOUT_GENERAL);

		UpdateAOUniform();
		const uint32_t GROUP_SIZE = 32;

		uint32_t groupX = (m_Width + (GROUP_SIZE - 1)) / GROUP_SIZE;
		uint32_t groupY = (m_Height + (GROUP_SIZE - 1)) / GROUP_SIZE;

		m_AOComputePipeline->Execute(primaryBuffer, groupX, groupY, 1, frameIndex);

		UpdateMeanUniform();
		m_MeanHorizontalComputePipeline->Execute(primaryBuffer, groupX, groupY, 1, frameIndex);
		m_MeanVerticalComputePipeline->Execute(primaryBuffer, groupX, groupY, 1, frameIndex);

		primaryBuffer->Blit(m_MeanVarianceTarget[1]->GetFrameBuffer(), m_MeanVarianceTarget[0]->GetFrameBuffer());

		m_AOTemporalPipeline->Execute(primaryBuffer, groupX, groupY, 1, frameIndex);

		primaryBuffer->Blit(m_RenderTarget[1]->GetFrameBuffer(), m_RenderTarget[0]->GetFrameBuffer());
		primaryBuffer->Blit(m_NormalDepthTarget[1]->GetFrameBuffer(), m_NormalDepthTarget[0]->GetFrameBuffer());

		m_AtrousComputePipeline->Execute(primaryBuffer, groupX, groupY, 1, frameIndex);

		m_ComposePipeline->Execute(primaryBuffer, groupX, groupY, 1, frameIndex);
	}
	return true;
}

bool KRTAO::ReloadShader()
{
	if (m_AOComputePipeline)
	{
		m_AOComputePipeline->ReloadShader();
		m_AOTemporalPipeline->ReloadShader();
		m_MeanHorizontalComputePipeline->ReloadShader();
		m_MeanVerticalComputePipeline->ReloadShader();
		m_AtrousComputePipeline->ReloadShader();
		m_ComposePipeline->ReloadShader();
		return true;
	}
	return false;
}

void KRTAO::UpdateSize()
{
	if (m_RenderTarget[0])
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

		m_NormalDepthTarget[0]->UnInit();
		m_NormalDepthTarget[0]->InitFromStorage(m_Width, m_Height, 1, 1, EF_R32G32B32A32_FLOAT);
		m_NormalDepthTarget[1]->UnInit();
		m_NormalDepthTarget[1]->InitFromStorage(m_Width, m_Height, 1, 1, EF_R32G32B32A32_FLOAT);

		m_RenderTarget[0]->UnInit();
		m_RenderTarget[0]->InitFromStorage(m_Width, m_Height, 1, 1, EF_R32G32_FLOAT);
		m_RenderTarget[1]->UnInit();
		m_RenderTarget[1]->InitFromStorage(m_Width, m_Height, 1, 1, EF_R32G32_FLOAT);

		m_CurrentTarget->UnInit();
		m_CurrentTarget->InitFromStorage(m_Width, m_Height, 1, 1, EF_R16G16_FLOAT);

		m_AtrousTarget->UnInit();
		m_AtrousTarget->InitFromStorage(m_Width, m_Height, 1, 1, EF_R16_FLOAT);

		m_ComposedTarget->UnInit();
		m_ComposedTarget->InitFromStorage(m_Width, m_Height, 1, 1, EF_R16G16B16A16_FLOAT);

		m_TemporalMeanSqaredMean->UnInit();
		m_TemporalMeanSqaredMean->InitFromStorage(m_Width, m_Height, 1, 1, EF_R16G16_FLOAT);

		m_MeanVarianceTarget[0]->UnInit();
		m_MeanVarianceTarget[0]->InitFromStorage(m_Width, m_Height, 1, 1, EF_R16G16_FLOAT);
		m_MeanVarianceTarget[1]->UnInit();
		m_MeanVarianceTarget[1]->InitFromStorage(m_Width, m_Height, 1, 1, EF_R16G16_FLOAT);
	}
}