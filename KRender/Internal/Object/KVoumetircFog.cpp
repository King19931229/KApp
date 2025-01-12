#include "KVolumetricFog.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKComputePipeline.h"
#include "Internal/KRenderGlobal.h"

KVolumetricFog::KVolumetricFog()
	: m_MainCamera(nullptr)
	, m_CurrentVoxelIndex(0)
	, m_GridX(0)
	, m_GridY(0)
	, m_GridZ(0)
	, m_Width(0)
	, m_Height(0)
	, m_Anisotropy(0.5f)
	, m_Density(0.5f)
	, m_Scattering(0.0f)
	, m_Absorption(0.0f)
	, m_Start(1.0f)
	, m_Depth(0)
	, m_Enable(true)
{
}

KVolumetricFog::~KVolumetricFog()
{
}

void KVolumetricFog::InitializePipeline()
{
	KRenderGlobal::ImmediateCommandList.BeginRecord();
	for (uint32_t cascadedIndex = 0; cascadedIndex < KRenderGlobal::CascadedShadowMap.GetNumCascaded(); ++cascadedIndex)
	{
		IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, true);
		if (shadowRT) KRenderGlobal::ImmediateCommandList.Transition(shadowRT->GetFrameBuffer(), PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}
	for (uint32_t cascadedIndex = 0; cascadedIndex < KRenderGlobal::CascadedShadowMap.GetNumCascaded(); ++cascadedIndex)
	{
		IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, false);
		if (shadowRT) KRenderGlobal::ImmediateCommandList.Transition(shadowRT->GetFrameBuffer(), PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}
	KRenderGlobal::ImmediateCommandList.EndRecord();

	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
	IKUniformBufferPtr staticShadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_STATIC_CASCADED_SHADOW);
	IKUniformBufferPtr dynamicShadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_DYNAMIC_CASCADED_SHADOW);
	IKSamplerPtr csmSampler = KRenderGlobal::CascadedShadowMap.GetSampler();

	for (uint32_t i = 0; i < 2; ++i)
	{
		m_VoxelLightInjectPipeline[i]->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
		m_VoxelLightInjectPipeline[i]->BindUniformBuffer(CBT_CAMERA, cameraBuffer);
		m_VoxelLightInjectPipeline[i]->BindUniformBuffer(CBT_GLOBAL, globalBuffer);
		m_VoxelLightInjectPipeline[i]->BindUniformBuffer(CBT_STATIC_CASCADED_SHADOW, staticShadowBuffer);
		m_VoxelLightInjectPipeline[i]->BindUniformBuffer(CBT_DYNAMIC_CASCADED_SHADOW, dynamicShadowBuffer);
		m_VoxelLightInjectPipeline[i]->BindSampler(VOLUMETRIC_FOG_BINDING_VOXEL_PREV, m_VoxelLightTarget[(i + 1) & 1]->GetFrameBuffer(), *m_VoxelSampler, false);
		m_VoxelLightInjectPipeline[i]->BindStorageImage(VOLUMETRIC_FOG_BINDING_VOXEL_CURR, m_VoxelLightTarget[i]->GetFrameBuffer(), VOXEL_FORMAT, COMPUTE_RESOURCE_OUT, 0, false);

		for (uint32_t cascadedIndex = 0; cascadedIndex < KRenderGlobal::CascadedShadowMap.SHADOW_MAP_MAX_CASCADED; ++cascadedIndex)
		{
			IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, true);
			if (!shadowRT) shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, true);
			m_VoxelLightInjectPipeline[i]->BindSampler(VOLUMETRIC_FOG_BINDING_STATIC_CSM0 + cascadedIndex, shadowRT->GetFrameBuffer(), csmSampler, false);
		}

		for (uint32_t cascadedIndex = 0; cascadedIndex < KRenderGlobal::CascadedShadowMap.SHADOW_MAP_MAX_CASCADED; ++cascadedIndex)
		{
			IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, false);
			if (!shadowRT) shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, true);
			m_VoxelLightInjectPipeline[i]->BindSampler(VOLUMETRIC_FOG_BINDING_DYNAMIC_CSM0 + cascadedIndex, shadowRT->GetFrameBuffer(), csmSampler, false);
		}

		m_VoxelLightInjectPipeline[i]->Init("volumetricfog/inject_light.comp", KShaderCompileEnvironment());
	}

	for (uint32_t i = 0; i < 2; ++i)
	{
		m_RayMatchPipeline[i]->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
		m_RayMatchPipeline[i]->BindSampler(VOLUMETRIC_FOG_BINDING_VOXEL_CURR, m_VoxelLightTarget[i]->GetFrameBuffer(), *m_VoxelSampler, false);
		m_RayMatchPipeline[i]->BindStorageImage(VOLUMETRIC_FOG_BINDING_VOXEL_RESULT, m_RayMatchResultTarget->GetFrameBuffer(), VOXEL_FORMAT, COMPUTE_RESOURCE_OUT, 0, false);
		m_RayMatchPipeline[i]->Init("volumetricfog/raymatch.comp", KShaderCompileEnvironment());
	}

	m_ScatteringPipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	m_ScatteringPipeline->SetShader(ST_VERTEX, *m_QuadVS);
	m_ScatteringPipeline->SetShader(ST_FRAGMENT, *m_ScatteringFS);

	m_ScatteringPipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	m_ScatteringPipeline->SetBlendEnable(false);
	m_ScatteringPipeline->SetCullMode(CM_NONE);
	m_ScatteringPipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	m_ScatteringPipeline->SetPolygonMode(PM_FILL);
	m_ScatteringPipeline->SetColorWrite(true, true, true, true);
	m_ScatteringPipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

	m_ScatteringPipeline->SetSampler(VOLUMETRIC_FOG_BINDING_GBUFFER_RT0, KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(), KRenderGlobal::GBuffer.GetSampler(), true);
	m_ScatteringPipeline->SetSampler(VOLUMETRIC_FOG_BINDING_VOXEL_RESULT, m_RayMatchResultTarget->GetFrameBuffer(), *m_VoxelSampler, false);
	m_ScatteringPipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

	m_ScatteringPipeline->Init();

	KRenderGlobal::ImmediateCommandList.BeginRecord();
	for (uint32_t cascadedIndex = 0; cascadedIndex < KRenderGlobal::CascadedShadowMap.GetNumCascaded(); ++cascadedIndex)
	{
		IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, true);
		if (shadowRT) KRenderGlobal::ImmediateCommandList.Transition(shadowRT->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
	}
	for (uint32_t cascadedIndex = 0; cascadedIndex < KRenderGlobal::CascadedShadowMap.GetNumCascaded(); ++cascadedIndex)
	{
		IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascadedIndex, false);
		if (shadowRT) KRenderGlobal::ImmediateCommandList.Transition(shadowRT->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
	}
	KRenderGlobal::ImmediateCommandList.EndRecord();
}

void KVolumetricFog::Resize(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;

	m_ScatteringTarget->UnInit();
	ASSERT_RESULT(m_ScatteringTarget->InitFromColor(width, height, 1, 1, EF_R8G8B8A8_UNORM));

	m_ScatteringPass->UnInit();
	m_ScatteringPass->SetColorAttachment(0, m_ScatteringTarget->GetFrameBuffer());
	m_ScatteringPass->SetOpColor(0, LO_CLEAR, SO_STORE);
	m_ScatteringPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 1.0f });
	ASSERT_RESULT(m_ScatteringPass->Init());
}

bool KVolumetricFog::Init(uint32_t gridX, uint32_t gridY, uint32_t gridZ, float depth, uint32_t width, uint32_t height, const KCamera* camera)
{
	UnInit();

	m_GridX = gridX;
	m_GridY = gridY;
	m_GridZ = gridZ;
	m_Depth = depth;
	m_MainCamera = camera;

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "volumetricfog/scattering.frag", m_ScatteringFS, false);

	KSamplerDescription desc;
	desc.minFilter = desc.magFilter = FM_LINEAR;
	desc.addressU = desc.addressV = desc.addressW = AM_CLAMP_TO_EDGE;
	KRenderGlobal::SamplerManager.Acquire(desc, m_VoxelSampler);

	for (uint32_t i = 0; i < 2; ++i)
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_VoxelLightTarget[i]);
		m_VoxelLightTarget[i]->InitFromStorage3D(m_GridX, m_GridY, m_GridZ, 1, VOXEL_FORMAT);
	}

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_RayMatchResultTarget);
	m_RayMatchResultTarget->InitFromStorage3D(m_GridX, m_GridY, m_GridZ, 1, VOXEL_FORMAT);

	for (uint32_t i = 0; i < 2; ++i)
	{
		KRenderGlobal::RenderDevice->CreateComputePipeline(m_VoxelLightInjectPipeline[i]);
		KRenderGlobal::RenderDevice->CreateComputePipeline(m_RayMatchPipeline[i]);
	}

	KRenderGlobal::RenderDevice->CreatePipeline(m_ScatteringPipeline);

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_ScatteringTarget);
	KRenderGlobal::RenderDevice->CreateRenderPass(m_ScatteringPass);

	Resize(width, height);
	InitializePipeline();

	m_PrevData.inited = false;

	return true;
}

bool KVolumetricFog::UnInit()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		SAFE_UNINIT(m_VoxelLightTarget[i]);
		SAFE_UNINIT(m_VoxelLightInjectPipeline[i]);
		SAFE_UNINIT(m_RayMatchPipeline[i]);
	}
	SAFE_UNINIT(m_RayMatchResultTarget);
	SAFE_UNINIT(m_ScatteringPipeline);
	SAFE_UNINIT(m_ScatteringTarget);
	SAFE_UNINIT(m_ScatteringPass);
	m_CurrentVoxelIndex = 0;
	m_MainCamera = nullptr;
	m_PrevData.inited = false;
	m_QuadVS.Release();
	m_ScatteringFS.Release();
	m_VoxelSampler.Release();
	return true;
}

void KVolumetricFog::UpdateVoxel(KRHICommandList& commandList)
{
	KCamera camera = *m_MainCamera;
	camera.SetNear(camera.GetNear() + std::max(0.1f, m_Start));
	camera.SetFar(camera.GetNear() + m_Depth);

	m_ObjectData.frameNum = KRenderGlobal::CurrentFrameNum;

	m_ObjectData.view = camera.GetViewMatrix();
	m_ObjectData.proj = camera.GetProjectiveMatrix();
	m_ObjectData.viewProj = m_ObjectData.proj * m_ObjectData.view;
	m_ObjectData.invViewProj = glm::inverse(m_ObjectData.viewProj);

	m_ObjectData.nearFarGridZ = glm::vec4(camera.GetNear(), camera.GetFar(), m_GridZ, 0);
	m_ObjectData.anisotropyDensityScatteringAbsorption = glm::vec4(m_Anisotropy, m_Density, m_Scattering, m_Absorption);
	m_ObjectData.cameraPos = glm::vec4(camera.GetPosition(), 0);

	if (m_PrevData.inited)
	{
		m_ObjectData.prevView = m_PrevData.view;
		m_ObjectData.prevProj = m_PrevData.proj;
		m_ObjectData.prevViewProj = m_PrevData.viewProj;
		m_ObjectData.prevInvViewProj = m_PrevData.invViewProj;
	}
	else
	{
		m_ObjectData.prevView = m_ObjectData.view;
		m_ObjectData.prevProj = m_ObjectData.proj;
		m_ObjectData.prevViewProj = m_ObjectData.viewProj;
		m_ObjectData.prevInvViewProj = m_ObjectData.invViewProj;
	}

	glm::uvec3 group = (glm::uvec3(m_GridX, m_GridY, m_GridZ) + glm::uvec3(GROUP_SIZE - 1)) / glm::uvec3(GROUP_SIZE);

	{
		commandList.BeginDebugMarker("InjectLight", glm::vec4(0, 1, 1, 0));
		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(m_ObjectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&m_ObjectData, objectUsage);

		commandList.Execute(m_VoxelLightInjectPipeline[m_CurrentVoxelIndex], group.x, group.y, group.z, &objectUsage);
		commandList.EndDebugMarker();
	}

	{
		commandList.BeginDebugMarker("RayMatch", glm::vec4(0, 1, 1, 0));
		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(m_ObjectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&m_ObjectData, objectUsage);

		commandList.Execute(m_RayMatchPipeline[m_CurrentVoxelIndex], group.x, group.y, 1, &objectUsage);
		commandList.EndDebugMarker();
	}

	m_PrevData.view = m_ObjectData.view;
	m_PrevData.proj = m_ObjectData.proj;
	m_PrevData.viewProj = m_ObjectData.viewProj;
	m_PrevData.invViewProj = m_ObjectData.invViewProj;
	m_PrevData.inited = true;
}

void KVolumetricFog::UpdateScattering(KRHICommandList& commandList)
{
	commandList.BeginDebugMarker("Scattering", glm::vec4(0, 1, 1, 0));
	commandList.BeginRenderPass(m_ScatteringPass, SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_ScatteringPass->GetViewPort());

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = m_ScatteringPipeline;
	command.pipeline->GetHandle(m_ScatteringPass, command.pipelineHandle);
	command.indexDraw = true;

	KDynamicConstantBufferUsage objectUsage;
	objectUsage.binding = SHADER_BINDING_OBJECT;
	objectUsage.range = sizeof(m_ObjectData);

	KRenderGlobal::DynamicConstantBufferManager.Alloc(&m_ObjectData, objectUsage);

	command.dynamicConstantUsages.push_back(objectUsage);
	
	commandList.Render(command);
	commandList.EndRenderPass();

	commandList.Transition(m_ScatteringTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	commandList.EndDebugMarker();
}

bool KVolumetricFog::Execute(KRHICommandList& commandList)
{
	if (!m_MainCamera)
		return false;

	commandList.BeginDebugMarker("VolumetricFog", glm::vec4(0, 1, 1, 0));

	if (m_Enable)
	{
		UpdateVoxel(commandList);
		UpdateScattering(commandList);
	}
	else
	{
		commandList.BeginDebugMarker("Scattering", glm::vec4(0, 1, 1, 0));
		commandList.BeginRenderPass(m_ScatteringPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(m_ScatteringPass->GetViewPort());
		commandList.EndRenderPass();
		commandList.Transition(m_ScatteringTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		commandList.EndDebugMarker();
	}

	m_CurrentVoxelIndex ^= 1;

	commandList.EndDebugMarker();

	return true;
}

void KVolumetricFog::Reload()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		m_VoxelLightInjectPipeline[i]->Reload();
		m_RayMatchPipeline[i]->Reload();
	}
	if (m_QuadVS)
		m_QuadVS->Reload();
	if (m_ScatteringFS)
		m_ScatteringFS->Reload();
	if(m_ScatteringPipeline)
		m_ScatteringPipeline->Reload(false);
}