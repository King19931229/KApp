#include "KVoxilzer.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"

const VertexFormat KVoxilzer::ms_VertexFormats[1] = { VF_DEBUG_POINT };

const VertexFormat KVoxilzer::ms_QuadFormats[] = { VF_SCREENQUAD_POS };

const KVertexDefinition::SCREENQUAD_POS_2F KVoxilzer::ms_QuadVertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint16_t KVoxilzer::ms_QuadIndices[] = { 0, 1, 2, 2, 3, 0 };

KVoxilzer::KVoxilzer()
	: m_Scene(nullptr)
	, m_StaticFlag(nullptr)
	, m_VoxelAlbedo(nullptr)
	, m_VoxelNormal(nullptr)
	, m_VoxelEmissive(nullptr)
	, m_VoxelRadiance(nullptr)
	, m_VolumeDimension(64)
	, m_VoxelCount(0)
	, m_NumMipmap(1)
	, m_VolumeGridSize(0)
	, m_VoxelSize(0)
	, m_InjectFirstBounce(true)
	, m_VoxelDrawEnable(true)
	, m_VoxelDebugUpdate(false)
{
	m_OnSceneChangedFunc = std::bind(&KVoxilzer::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KVoxilzer::~KVoxilzer()
{
}

void KVoxilzer::UpdateInternal()
{
	UpdateProjectionMatrices();

	m_PrimaryCommandBuffer->BeginPrimary();

	VoxelizeStaticScene(m_PrimaryCommandBuffer);
	UpdateRadiance(m_PrimaryCommandBuffer);

	m_PrimaryCommandBuffer->End();
	m_PrimaryCommandBuffer->Flush();
}

void KVoxilzer::OnSceneChanged(EntitySceneOp op, IKEntityPtr entity)
{
	// TODO 每帧只更新一次
	UpdateInternal();
}

void KVoxilzer::Update()
{
	if (m_VoxelDebugUpdate)
	{
		UpdateInternal();
	}
}

void KVoxilzer::ReloadShader()
{
	m_VoxelDrawVS->Reload();
	m_VoxelDrawGS->Reload();
	m_VoxelDrawFS->Reload();
	for (IKPipelinePtr& pipeline : m_VoxelDrawPipelines)
	{
		pipeline->Reload();
	}
	m_LightPassVS->Reload();
	m_LightPassFS->Reload();
	for (IKPipelinePtr& pipeline : m_LightPassPipelines)
	{
		pipeline->Reload();
	}
	m_InjectRadiancePipeline->ReloadShader();
	m_InjectPropagationPipeline->ReloadShader();
	m_MipmapBasePipeline->ReloadShader();
	m_MipmapVolumePipeline->ReloadShader();
}

void KVoxilzer::UpdateProjectionMatrices()
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	glm::vec3 axisSize = sceneBox.GetExtend();
	const glm::vec3& center = sceneBox.GetCenter();
	const glm::vec3 min = sceneBox.GetMin();
	const glm::vec3 max = sceneBox.GetMax();

	m_VolumeGridSize = glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
	m_VoxelSize = m_VolumeGridSize / m_VolumeDimension;
	float halfSize = m_VolumeGridSize / 2.0f;

	// projection matrices
	glm::mat4 projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, m_VolumeGridSize);

	// view matrices
	m_ViewProjectionMatrix[0] = lookAt(center + glm::vec3(halfSize, 0.0f, 0.0f),
		center, glm::vec3(0.0f, 1.0f, 0.0f));
	m_ViewProjectionMatrix[1] = lookAt(center + glm::vec3(0.0f, halfSize, 0.0f),
		center, glm::vec3(0.0f, 0.0f, -1.0f));
	m_ViewProjectionMatrix[2] = lookAt(center + glm::vec3(0.0f, 0.0f, halfSize),
		center, glm::vec3(0.0f, 1.0f, 0.0f));

	for (uint32_t i = 0; i < 3; ++i)
	{
		m_ViewProjectionMatrix[i] = projection * m_ViewProjectionMatrix[i];
		m_ViewProjectionMatrixI[i] = glm::inverse(m_ViewProjectionMatrix[i]);
	}

	for (uint32_t frameIndex = 0; frameIndex < KRenderGlobal::RenderDevice->GetNumFramesInFlight(); ++frameIndex)
	{
		IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_VOXEL);

		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_VOXEL);
		const KConstantDefinition::ConstantBufferDetail& details = KConstantDefinition::GetConstantBufferDetail(CBT_VOXEL);

		for (const KConstantDefinition::ConstantSemanticDetail& detail : details.semanticDetails)
		{
			void* pWritePos = nullptr;
			pWritePos = POINTER_OFFSET(pData, detail.offset);
			if (detail.semantic == CS_VOXEL_VIEW_PROJ)
			{
				assert(sizeof(glm::mat4) * 3 == detail.size);
				for (size_t i = 0; i < 3; i++)
				{
					memcpy(pWritePos, &m_ViewProjectionMatrix[i], sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			if (detail.semantic == CS_VOXEL_VIEW_PROJ_INV)
			{
				assert(sizeof(glm::mat4) * 3 == detail.size);
				for (size_t i = 0; i < 3; i++)
				{
					memcpy(pWritePos, &m_ViewProjectionMatrixI[i], sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			if (detail.semantic == CS_VOXEL_SUNLIGHT)
			{
				assert(sizeof(glm::vec4) == detail.size);
				glm::vec4 sunLight(-KRenderGlobal::CascadedShadowMap.GetCamera().GetForward(), 0.0);
				memcpy(pWritePos, &sunLight, sizeof(glm::vec4));
			}
			if (detail.semantic == CS_VOXEL_MINPOINT_SCALE)
			{
				assert(sizeof(glm::vec4) == detail.size);
				glm::vec4 minpointScale(min, 1.0f / m_VolumeGridSize);
				memcpy(pWritePos, &minpointScale, sizeof(glm::vec4));
			}
			if (detail.semantic == CS_VOXEL_MAXPOINT_SCALE)
			{
				assert(sizeof(glm::vec4) == detail.size);
				glm::vec4 maxpointScale(max, 1.0f / m_VolumeGridSize);
				memcpy(pWritePos, &maxpointScale, sizeof(glm::vec4));
			}
			if(detail.semantic == CS_VOXEL_MISCS)
			{
				assert(sizeof(uint32_t) * 4 == detail.size);
				glm::uvec4 miscs(0);
				// volumeDimension
				miscs[0] = m_VolumeDimension;
				// storeVisibility
				miscs[1] = 1;
				// normalWeightedLambert
				miscs[2] = 1;
				// checkBoundaries
				miscs[3] = 1;
				memcpy(pWritePos, &miscs, sizeof(uint32_t) * 4);
			}
			if (detail.semantic == CS_VOXEL_MISCS2)
			{
				assert(sizeof(float) * 4 == detail.size);
				glm::vec4 miscs2(0);
				// voxelSize
				miscs2[0] = m_VoxelSize;
				// volumeSize
				miscs2[1] = m_VolumeGridSize;
				// exponents
				miscs2[2] = miscs2[3] = 0;
				memcpy(pWritePos, &miscs2, sizeof(float) * 4);
			}
			if (detail.semantic == CS_VOXEL_MISCS3)
			{
				assert(sizeof(float) * 4 == detail.size);
				glm::vec4 miscs3(0);
				// lightBleedingReduction
				miscs3[0] = 0;
				// traceShadowHit
				miscs3[1] = 0.5f;
				// maxTracingDistanceGlobal
				miscs3[2] = 1.0f;
				memcpy(pWritePos, &miscs3, sizeof(float) * 4);
			}
		}

		voxelBuffer->Write(pData);
	}
}

void KVoxilzer::SetupVoxelVolumes(uint32_t dimension)
{
	m_VolumeDimension = dimension;
	m_VoxelCount = m_VolumeDimension * m_VolumeDimension * m_VolumeDimension;
	m_VoxelSize = m_VolumeGridSize / m_VolumeDimension;

	uint32_t baseMipmapDimension = (dimension + 1) / 2;
	m_NumMipmap = (uint32_t)(std::floor(std::log(baseMipmapDimension) / std::log(2)) + 1);

	UpdateProjectionMatrices();

	m_VoxelAlbedo->InitFromStorage(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	m_VoxelNormal->InitFromStorage(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	m_VoxelRadiance->InitFromStorage(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	for (uint32_t i = 0; i < 6; ++i)
	{
		m_VoxelTexMipmap[i]->InitFromStorage(baseMipmapDimension, baseMipmapDimension, baseMipmapDimension, m_NumMipmap, EF_R8GB8BA8_UNORM);
	}
	m_VoxelEmissive->InitFromStorage(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	m_StaticFlag->InitFromStorage(dimension, dimension, dimension, 1, EF_R8_UNORM);

	m_CloestSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_CloestSampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	m_CloestSampler->Init(0, 0);

	m_LinearSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_LinearSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_LinearSampler->Init(0, 0);

	m_MipmapSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_MipmapSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_MipmapSampler->Init(0, m_NumMipmap - 1);

	m_RenderPassTarget->InitFromColor(dimension, dimension, 1, EF_R8_UNORM);
	m_RenderPass->SetColorAttachment(0, m_RenderPassTarget->GetFrameBuffer());
	m_RenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_RenderPass->Init();
}

void KVoxilzer::SetupVoxelDrawPipeline()
{
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/draw_voxels.vert", m_VoxelDrawVS, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/draw_voxels.geom", m_VoxelDrawGS, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/draw_voxels.frag", m_VoxelDrawFS, true));

	for (size_t frameIndex = 0; frameIndex < m_VoxelDrawPipelines.size(); ++frameIndex)
	{
		IKPipelinePtr& pipeline = m_VoxelDrawPipelines[frameIndex];

		pipeline->SetShader(ST_VERTEX, m_VoxelDrawVS);

		pipeline->SetPrimitiveTopology(PT_POINT_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetShader(ST_GEOMETRY, m_VoxelDrawGS);
		pipeline->SetShader(ST_FRAGMENT, m_VoxelDrawFS);

		IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_VOXEL);
		pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelBuffer);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, cameraBuffer);

		pipeline->SetStorageImage(VOXEL_BINDING_ALBEDO, KRenderGlobal::Voxilzer.GetVoxelAlbedo(), EF_UNKNOWN);
		pipeline->SetStorageImage(VOXEL_BINDING_NORMAL, KRenderGlobal::Voxilzer.GetVoxelNormal(), EF_UNKNOWN);
		pipeline->SetStorageImage(VOXEL_BINDING_EMISSION, KRenderGlobal::Voxilzer.GetVoxelEmissive(), EF_UNKNOWN);
		pipeline->SetStorageImage(VOXEL_BINDING_RADIANCE, KRenderGlobal::Voxilzer.GetVoxelRadiance(), EF_UNKNOWN);
		pipeline->SetStorageImage(VOXEL_BINDING_TEXMIPMAP_OUT, KRenderGlobal::Voxilzer.GetVoxelTexMipmap(0), EF_UNKNOWN);

		pipeline->Init();
	}

	m_VoxelDrawVertexData.vertexCount = (uint32_t)(m_VolumeDimension * m_VolumeDimension * m_VolumeDimension);
	m_VoxelDrawVertexData.vertexStart = 0;
}

void KVoxilzer::SetupRadiancePipeline()
{
	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_GLOBAL);
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_VOXEL);

	// Inject Radiance
	m_InjectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_GLOBAL, globalBuffer);
	m_InjectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);

	m_InjectRadiancePipeline->BindStorageImage(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_IMAGE_IN, 0, false);
	m_InjectRadiancePipeline->BindStorageImage(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_IMAGE_OUT, 0, false);
	m_InjectRadiancePipeline->BindStorageImage(VOXEL_BINDING_EMISSION_MAP, m_VoxelEmissive->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_IMAGE_IN, 0, false);
	m_InjectRadiancePipeline->BindSampler(VOXEL_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), m_LinearSampler, false);

	m_InjectRadiancePipeline->Init("voxel/inject_radiance.comp");

	// Inject Propagation
	m_InjectPropagationPipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);

	m_InjectPropagationPipeline->BindStorageImage(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_IMAGE_OUT, 0, false);
	m_InjectPropagationPipeline->BindSampler(VOXEL_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), m_LinearSampler, false);
	m_InjectPropagationPipeline->BindSampler(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), m_LinearSampler, false);

	std::vector<IKFrameBufferPtr> targets;
	std::vector<IKSamplerPtr> samplers;
	targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	samplers.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
	{
		targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
		samplers[i] = m_MipmapSampler;
	}

	m_InjectPropagationPipeline->BindSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers, false);

	m_InjectPropagationPipeline->Init("voxel/inject_propagation.comp");
}

void KVoxilzer::SetupMipmapPipeline()
{
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_VOXEL);

	{
		std::vector<IKFrameBufferPtr> targets;
		targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
		for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
		{
			targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
		}

		m_MipmapBasePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
		m_MipmapBasePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
		m_MipmapBasePipeline->BindSampler(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), m_LinearSampler, false);
		m_MipmapBasePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, EF_UNKNOWN, COMPUTE_IMAGE_OUT, 0, true);

		m_MipmapBasePipeline->Init("voxel/aniso_mipmapbase.comp");
	}

	{
		std::vector<IKFrameBufferPtr> targets;
		std::vector<IKSamplerPtr> samplers;
		targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
		samplers.resize(ARRAY_SIZE(m_VoxelTexMipmap));
		for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
		{
			targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
			samplers[i] = m_MipmapSampler;
		}

		m_MipmapVolumePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
		m_MipmapVolumePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
		m_MipmapVolumePipeline->BindSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers, false);
		m_MipmapVolumePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, EF_UNKNOWN, COMPUTE_IMAGE_OUT, 0, true);

		m_MipmapVolumePipeline->Init("voxel/aniso_mipmapvolume.comp");
	}
}

void KVoxilzer::SetupLightPassPipeline()
{
	m_QuadVertexBuffer->InitMemory(ARRAY_SIZE(ms_QuadVertices), sizeof(ms_QuadVertices[0]), ms_QuadVertices);
	m_QuadVertexBuffer->InitDevice(false);

	m_QuadIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_QuadIndices), ms_QuadIndices);
	m_QuadIndexBuffer->InitDevice(false);

	m_QuadVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_QuadVertexBuffer);
	m_QuadVertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
	m_QuadVertexData.vertexCount = ARRAY_SIZE(ms_QuadVertices);
	m_QuadVertexData.vertexStart = 0;

	m_QuadIndexData.indexBuffer = m_QuadIndexBuffer;
	m_QuadIndexData.indexCount = ARRAY_SIZE(ms_QuadIndices);
	m_QuadIndexData.indexStart = 0;

	std::vector<IKFrameBufferPtr> targets;
	std::vector<IKSamplerPtr> samplers;
	targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	samplers.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
	{
		targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
		samplers[i] = m_MipmapSampler;
	}

	for (size_t frameIndex = 0; frameIndex < m_LightPassPipelines.size(); ++frameIndex)
	{
		IKPipelinePtr& pipeline = m_LightPassPipelines[frameIndex];

		pipeline->SetVertexBinding(ms_QuadFormats, ARRAY_SIZE(ms_QuadFormats));
		pipeline->SetShader(ST_VERTEX, m_LightPassVS);
		pipeline->SetShader(ST_FRAGMENT, m_LightPassFS);

		pipeline->SetPrimitiveTopology(PT_POINT_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_VOXEL);
		pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelBuffer);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, cameraBuffer);

		pipeline->SetSampler(VOXEL_BINDING_GBUFFER_NORMAL,
			KRenderGlobal::GBuffer.GetGBufferTarget(KGBuffer::RT_NORMAL)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			false);
		pipeline->SetSampler(VOXEL_BINDING_GBUFFER_POSITION,
			KRenderGlobal::GBuffer.GetGBufferTarget(KGBuffer::RT_POSITION)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			false);
		pipeline->SetSampler(VOXEL_BINDING_GBUFFER_ALBEDO,
			KRenderGlobal::GBuffer.GetGBufferTarget(KGBuffer::RT_DIFFUSE)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			false);
		pipeline->SetSampler(VOXEL_BINDING_GBUFFER_SPECULAR,
			KRenderGlobal::GBuffer.GetGBufferTarget(KGBuffer::RT_SPECULAR)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			false);
		pipeline->SetSampler(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), m_LinearSampler, false);
		pipeline->SetSampler(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), m_LinearSampler, false);
		pipeline->SetSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers, false);

		pipeline->Init();
	}
}

void KVoxilzer::VoxelizeStaticScene(IKCommandBufferPtr commandBuffer)
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	std::vector<KRenderComponent*> cullRes;
	((KRenderScene*)m_Scene)->GetRenderComponent(sceneBox, cullRes);

	if (cullRes.size() == 0) return;

	commandBuffer->BeginRenderPass(m_RenderPass, SUBPASS_CONTENTS_INLINE);
	commandBuffer->SetViewport(m_RenderPass->GetViewPort());

	std::vector<KRenderCommand> commands;
	for (KRenderComponent* render : cullRes)
	{
		render->Visit(PIPELINE_STAGE_VOXEL, 0, [&](KRenderCommand& command)
		{
			IKEntity* entity = render->GetEntityHandle();

			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transform))
			{
				const glm::mat4& finalTran = transform->GetFinal();
				const glm::mat4& prevFinalTran = transform->GetPrevFinal();

				KConstantDefinition::OBJECT objectData;
				objectData.MODEL = finalTran;
				objectData.PRVE_MODEL = prevFinalTran;
				command.objectUsage.binding = SHADER_BINDING_OBJECT;
				command.objectUsage.range = sizeof(objectData);

				KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

				command.pipeline->GetHandle(m_RenderPass, command.pipelineHandle);

				commands.push_back(command);
			}
		});
	}

	for (KRenderCommand& command : commands)
	{
		commandBuffer->Render(command);
	}

	commandBuffer->EndRenderPass();
}

void KVoxilzer::UpdateRadiance(IKCommandBufferPtr commandBuffer)
{
	InjectRadiance(commandBuffer);
	GenerateMipmap(commandBuffer);

	if (m_InjectFirstBounce)
	{
		uint32_t group = (m_VolumeDimension + (GROUP_SIZE - 1)) / GROUP_SIZE;
		m_InjectPropagationPipeline->Execute(commandBuffer, group, group, group, 0);

		GenerateMipmap(commandBuffer);
	}
}

void KVoxilzer::InjectRadiance(IKCommandBufferPtr commandBuffer)
{
	uint32_t group = (m_VolumeDimension + (GROUP_SIZE - 1)) / GROUP_SIZE;
	m_InjectRadiancePipeline->Execute(commandBuffer, group, group, group, 0);
}

void KVoxilzer::GenerateMipmap(IKCommandBufferPtr commandBuffer)
{
	GenerateMipmapBase(commandBuffer);
	GenerateMipmapVolume(commandBuffer);
}

void KVoxilzer::GenerateMipmapBase(IKCommandBufferPtr commandBuffer)
{
	uint32_t dimension = m_VolumeDimension / 2;
	uint32_t group = (dimension + (GROUP_SIZE - 1)) / GROUP_SIZE;

	glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, 0);

	KDynamicConstantBufferUsage usage;
	usage.binding = SHADER_BINDING_OBJECT;
	usage.range = sizeof(constant);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

	m_MipmapBasePipeline->Execute(commandBuffer, group, group, group, 0, &usage);
}

void KVoxilzer::GenerateMipmapVolume(IKCommandBufferPtr commandBuffer)
{
	uint32_t dimension = m_VolumeDimension / 4;
	uint32_t mipmap = 1;

	std::vector<IKFrameBufferPtr> targets;
	targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
	{
		targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
	}

	while (dimension > 0)
	{
		uint32_t group = (dimension + (GROUP_SIZE - 1)) / GROUP_SIZE;

		for (size_t idx = 0; idx < ARRAY_SIZE(m_VoxelTexMipmap); ++idx)
		{
			glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, mipmap - 1);

			KDynamicConstantBufferUsage usage;
			usage.binding = SHADER_BINDING_OBJECT;
			usage.range = sizeof(constant);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

			m_MipmapVolumePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, EF_UNKNOWN, COMPUTE_IMAGE_OUT, mipmap, true);
			m_MipmapVolumePipeline->Execute(commandBuffer, group, group, group, 0, &usage);
		}

		dimension /= 2;
		++mipmap;
	}

	ASSERT_RESULT(mipmap == m_NumMipmap);
}

bool KVoxilzer::RenderVoxel(size_t frameIndex, IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers)
{
	if (!m_VoxelDrawEnable)
		return true;

	if (frameIndex < m_VoxelDrawPipelines.size())
	{
		KRenderCommand command;
		command.vertexData = &m_VoxelDrawVertexData;
		command.indexData = nullptr;
		command.pipeline = m_VoxelDrawPipelines[frameIndex];
		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = false;

		IKCommandBufferPtr commandBuffer = m_CommandBuffers[frameIndex];

		commandBuffer->BeginSecondary(renderPass);
		commandBuffer->SetViewport(renderPass->GetViewPort());
		commandBuffer->Render(command);
		commandBuffer->End();

		buffers.push_back(commandBuffer);
		return true;
	}
	return false;
}

bool KVoxilzer::Init(IKRenderScene* scene, uint32_t dimension)
{
	UnInit();

	m_Scene = scene;

	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	renderDevice->CreateVertexBuffer(m_QuadVertexBuffer);
	renderDevice->CreateIndexBuffer(m_QuadIndexBuffer);

	renderDevice->CreateRenderTarget(m_StaticFlag);
	renderDevice->CreateRenderTarget(m_VoxelAlbedo);
	renderDevice->CreateRenderTarget(m_VoxelNormal);
	renderDevice->CreateRenderTarget(m_VoxelEmissive);
	renderDevice->CreateRenderTarget(m_VoxelRadiance);

	renderDevice->CreateRenderTarget(m_LightPassTarget);

	for (uint32_t i = 0; i < 6; ++i)
	{
		renderDevice->CreateRenderTarget(m_VoxelTexMipmap[i]);
	}

	renderDevice->CreateSampler(m_CloestSampler);
	renderDevice->CreateSampler(m_LinearSampler);
	renderDevice->CreateSampler(m_MipmapSampler);

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	renderDevice->CreateCommandBuffer(m_PrimaryCommandBuffer);
	m_PrimaryCommandBuffer->Init(m_CommandPool, CBL_PRIMARY);

	renderDevice->CreateRenderTarget(m_RenderPassTarget);
	renderDevice->CreateRenderPass(m_RenderPass);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/light_pass.vert", m_LightPassVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/light_pass.frag", m_LightPassFS, false);

	m_VoxelDrawPipelines.resize(renderDevice->GetNumFramesInFlight());
	m_LightPassPipelines.resize(renderDevice->GetNumFramesInFlight());
	m_CommandBuffers.resize(renderDevice->GetNumFramesInFlight());

	for (size_t frameIndex = 0; frameIndex < m_VoxelDrawPipelines.size(); ++frameIndex)
	{
		renderDevice->CreatePipeline(m_VoxelDrawPipelines[frameIndex]);
		renderDevice->CreateCommandBuffer(m_CommandBuffers[frameIndex]);
		m_CommandBuffers[frameIndex]->Init(m_CommandPool, CBL_SECONDARY);
	}

	for (size_t frameIndex = 0; frameIndex < m_LightPassPipelines.size(); ++frameIndex)
	{
		renderDevice->CreatePipeline(m_LightPassPipelines[frameIndex]);
	}

	renderDevice->CreateComputePipeline(m_InjectRadiancePipeline);
	renderDevice->CreateComputePipeline(m_InjectPropagationPipeline);

	renderDevice->CreateComputePipeline(m_MipmapBasePipeline);
	renderDevice->CreateComputePipeline(m_MipmapVolumePipeline);

	SetupVoxelVolumes(dimension);
	SetupVoxelDrawPipeline();
	SetupRadiancePipeline();
	SetupMipmapPipeline();
	SetupLightPassPipeline();

	m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);

	return true;
}

bool KVoxilzer::UnInit()
{
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
	}
	m_Scene = nullptr;

	KRenderGlobal::ShaderManager.Release(m_LightPassVS);
	m_LightPassVS = nullptr;
	KRenderGlobal::ShaderManager.Release(m_LightPassFS);
	m_LightPassFS = nullptr;

	SAFE_UNINIT(m_QuadIndexBuffer);
	SAFE_UNINIT(m_QuadVertexBuffer);

	SAFE_UNINIT(m_LightPassTarget);

	SAFE_UNINIT_CONTAINER(m_VoxelDrawPipelines);
	SAFE_UNINIT_CONTAINER(m_LightPassPipelines);

	SAFE_UNINIT(m_InjectRadiancePipeline);
	SAFE_UNINIT(m_InjectPropagationPipeline);

	SAFE_UNINIT(m_MipmapBasePipeline);
	SAFE_UNINIT(m_MipmapVolumePipeline);

	SAFE_UNINIT(m_VoxelDrawVS);
	SAFE_UNINIT(m_VoxelDrawGS);
	SAFE_UNINIT(m_VoxelDrawFS);

	SAFE_UNINIT(m_RenderPass);
	SAFE_UNINIT(m_RenderPassTarget);
	SAFE_UNINIT_CONTAINER(m_CommandBuffers);
	SAFE_UNINIT(m_PrimaryCommandBuffer);
	SAFE_UNINIT(m_CommandPool);

	SAFE_UNINIT(m_StaticFlag);
	SAFE_UNINIT(m_VoxelAlbedo);
	SAFE_UNINIT(m_VoxelNormal);
	SAFE_UNINIT(m_VoxelEmissive);
	SAFE_UNINIT(m_VoxelRadiance);

	for (uint32_t i = 0; i < 6; ++i)
	{
		SAFE_UNINIT(m_VoxelTexMipmap[i]);
	}

	SAFE_UNINIT(m_CloestSampler);
	SAFE_UNINIT(m_LinearSampler);
	SAFE_UNINIT(m_MipmapSampler);

	return true;
}