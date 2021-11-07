#include "KVoxilzer.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"

const VertexFormat KVoxilzer::ms_VertexFormats[1] = { VF_DEBUG_POINT };

KVoxilzer::KVoxilzer()
	: m_Scene(nullptr)
	, m_StaticFlag(nullptr)
	, m_VoxelAlbedo(nullptr)
	, m_VoxelNormal(nullptr)
	, m_VoxelEmissive(nullptr)
	, m_VoxelRadiance(nullptr)
	, m_VolumeDimension(256)
	, m_VoxelCount(0)
	, m_NumMipmap(1)
	, m_VolumeGridSize(0)
	, m_VoxelSize(0)
	, m_VoxelDrawEnable(true)
{
	m_OnSceneChangedFunc = std::bind(&KVoxilzer::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KVoxilzer::~KVoxilzer()
{
}

void KVoxilzer::OnSceneChanged(EntitySceneOp op, IKEntityPtr entity)
{
	UpdateProjectionMatrices();
	VoxelizeStaticScene();
	UpdateRadiance();
}

void KVoxilzer::UpdateProjectionMatrices()
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	glm::vec3 axisSize = sceneBox.GetExtend();
	const glm::vec3& center = sceneBox.GetCenter();
	const glm::vec3 min = sceneBox.GetMin();

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
			if (detail.semantic == CS_VOXEL_MINPOINT_SCALE)
			{
				assert(sizeof(glm::vec4) == detail.size);
				glm::vec4 midpointScale(min, 1.0f / m_VolumeGridSize);
				memcpy(pWritePos, &midpointScale, sizeof(glm::vec4));
			}
			if(detail.semantic == CS_VOXEL_MISCS)
			{
				assert(sizeof(uint32_t) * 4 == detail.size);
				glm::uvec4 miscs(0);
				// volumeDimension
				miscs.x = m_VolumeDimension;
				// storeVisibility
				miscs.y = 1;
				// normalWeightedLambert
				miscs.z = 1;
				// checkBoundaries
				miscs.w = 1;
				memcpy(pWritePos, &miscs, sizeof(uint32_t) * 4);
			}
			if (detail.semantic == CS_VOXEL_MISCS2)
			{
				assert(sizeof(float) * 4 == detail.size);
				glm::vec4 miscs2(0);
				// voxelSize
				miscs2.x = m_VoxelSize;
				// volumeSize
				miscs2.y = m_VolumeGridSize;
				// exponents
				miscs2.z = miscs2.w = 0;
				memcpy(pWritePos, &miscs2, sizeof(float) * 4);
			}
			if (detail.semantic == CS_VOXEL_MISCS3)
			{
				assert(sizeof(float) * 4 == detail.size);
				glm::vec4 miscs3(0);
				// lightBleedingReduction
				miscs3.x = 0;
				// traceShadowHit
				miscs3.y = 0.5f;
				// maxTracingDistanceGlobal
				miscs3.w = 1.0f;
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
	m_NumMipmap = (uint32_t)(std::floor(std::log(dimension) / std::log(2)) + 1);

	UpdateProjectionMatrices();

	m_VoxelAlbedo->InitFromStorage(dimension, dimension, dimension, 1, EF_R32_UINT);
	m_VoxelNormal->InitFromStorage(dimension, dimension, dimension, 1, EF_R32_UINT);
	m_VoxelRadiance->InitFromStorage(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	for (uint32_t i = 0; i < 6; ++i)
	{
		m_VoxelTexMipmap[i]->InitFromStorage(dimension, dimension, dimension, m_NumMipmap, EF_R8GB8BA8_UNORM);
	}
	m_VoxelEmissive->InitFromStorage(dimension, dimension, dimension, 1, EF_R32_UINT);
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

		pipeline->SetStorageImage(VOXEL_BINDING_ALBEDO, KRenderGlobal::Voxilzer.GetVoxelAlbedo());
		pipeline->SetStorageImage(VOXEL_BINDING_NORMAL, KRenderGlobal::Voxilzer.GetVoxelNormal());
		pipeline->SetStorageImage(VOXEL_BINDING_EMISSION, KRenderGlobal::Voxilzer.GetVoxelEmissive());
		pipeline->SetStorageImage(VOXEL_BINDING_RADIANCE, KRenderGlobal::Voxilzer.GetVoxelRadiance());
		pipeline->SetStorageImage(VOXEL_BINDING_TEXMIPMAP_OUT, KRenderGlobal::Voxilzer.GetVoxelTexMipmap(0));

		pipeline->Init();
	}

	m_VoxelDrawVertexData.vertexCount = (uint32_t)(m_VolumeDimension * m_VolumeDimension * m_VolumeDimension);
	m_VoxelDrawVertexData.vertexStart = 0;
}

void KVoxilzer::SetupRadiancePipeline()
{
	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_GLOBAL);
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_VOXEL);

	m_InjectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_GLOBAL, globalBuffer);
	m_InjectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);

	m_InjectRadiancePipeline->BindStorageImage(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), COMPUTE_IMAGE_IN);
	m_InjectRadiancePipeline->ReinterpretImageFormat(VOXEL_BINDING_NORMAL, EF_R8GB8BA8_UNORM);

	m_InjectRadiancePipeline->BindStorageImage(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), COMPUTE_IMAGE_OUT);

	m_InjectRadiancePipeline->BindStorageImage(VOXEL_BINDING_EMISSION_MAP, m_VoxelEmissive->GetFrameBuffer(), COMPUTE_IMAGE_IN);
	m_InjectRadiancePipeline->ReinterpretImageFormat(VOXEL_BINDING_EMISSION_MAP, EF_R8GB8BA8_UNORM);

	m_InjectRadiancePipeline->BindSampler(VOXEL_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), m_LinearSampler);
	m_InjectRadiancePipeline->ReinterpretImageFormat(VOXEL_BINDING_ALBEDO, EF_R8GB8BA8_UNORM);

	m_InjectRadiancePipeline->Init("voxel/inject_radiance.comp");
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

		m_MipmapBasePipeline->BindDyanmicUniformBuffer(SHADER_BINDING_OBJECT);
		m_MipmapBasePipeline->BindSampler(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), m_LinearSampler);
		m_MipmapBasePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, COMPUTE_IMAGE_OUT);

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

		m_MipmapVolumePipeline->BindDyanmicUniformBuffer(SHADER_BINDING_OBJECT);
		m_MipmapVolumePipeline->BindSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers);
		m_MipmapVolumePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, COMPUTE_IMAGE_OUT);

		m_MipmapVolumePipeline->Init("voxel/aniso_mipmapvolume.comp");
	}
}

void KVoxilzer::VoxelizeStaticScene()
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	std::vector<KRenderComponent*> cullRes;
	((KRenderScene*)m_Scene)->GetRenderComponent(sceneBox, cullRes);

	if (cullRes.size() == 0) return;

	m_PrimaryCommandBuffer->BeginPrimary();

	m_PrimaryCommandBuffer->BeginRenderPass(m_RenderPass, SUBPASS_CONTENTS_INLINE);
	m_PrimaryCommandBuffer->SetViewport(m_RenderPass->GetViewPort());

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
		m_PrimaryCommandBuffer->Render(command);
	}

	m_PrimaryCommandBuffer->EndRenderPass();

	m_PrimaryCommandBuffer->End();

	m_PrimaryCommandBuffer->Flush();
}

void KVoxilzer::UpdateRadiance()
{
	InjectRadiance();
	GenerateMipmap();
}

void KVoxilzer::InjectRadiance()
{
	uint32_t group = (m_VolumeDimension + (GROUP_SIZE - 1)) / GROUP_SIZE;

	m_PrimaryCommandBuffer->BeginPrimary();
	m_PrimaryCommandBuffer->TranslateToShader(m_VoxelAlbedo->GetFrameBuffer());
	m_InjectRadiancePipeline->Execute(m_PrimaryCommandBuffer, group, group, group, 0);
	m_PrimaryCommandBuffer->TranslateToStorage(m_VoxelAlbedo->GetFrameBuffer());
	m_PrimaryCommandBuffer->End();

	m_PrimaryCommandBuffer->Flush();
}

void KVoxilzer::GenerateMipmap()
{
	GenerateMipmapBase();
	GenerateMipmapVolume();
}

void KVoxilzer::GenerateMipmapBase()
{
	uint32_t dimension = m_VolumeDimension / 2;
	uint32_t group = (dimension + (GROUP_SIZE - 1)) / GROUP_SIZE;

	m_PrimaryCommandBuffer->BeginPrimary();
	m_PrimaryCommandBuffer->TranslateToShader(m_VoxelRadiance->GetFrameBuffer());

	glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, 0);

	KDynamicConstantBufferUsage usage;
	usage.binding = SHADER_BINDING_OBJECT;
	usage.range = sizeof(constant);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

	m_MipmapBasePipeline->Execute(m_PrimaryCommandBuffer, group, group, group, 0, &usage);

	m_PrimaryCommandBuffer->TranslateToStorage(m_VoxelRadiance->GetFrameBuffer());
	m_PrimaryCommandBuffer->End();

	m_PrimaryCommandBuffer->Flush();
}

void KVoxilzer::GenerateMipmapVolume()
{
	uint32_t dimension = m_VolumeDimension / 4;
	uint32_t mipmap = 1;

	m_PrimaryCommandBuffer->BeginPrimary();

	while (dimension > 0)
	{
		uint32_t group = (dimension + (GROUP_SIZE - 1)) / GROUP_SIZE;

		for (size_t idx = 0; idx < ARRAY_SIZE(m_VoxelTexMipmap); ++idx)
		{
			m_PrimaryCommandBuffer->TranslateToShader(m_VoxelTexMipmap[idx]->GetFrameBuffer());

			glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, mipmap);

			KDynamicConstantBufferUsage usage;
			usage.binding = SHADER_BINDING_OBJECT;
			usage.range = sizeof(constant);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

			// m_MipmapVolumePipeline->Execute(m_PrimaryCommandBuffer, group, group, group, 0, &usage);

			m_PrimaryCommandBuffer->TranslateToStorage(m_VoxelTexMipmap[idx]->GetFrameBuffer());
		}

		dimension /= 2;
		++mipmap;
	}

	ASSERT_RESULT(mipmap + 1 == m_NumMipmap);

	m_PrimaryCommandBuffer->End();
	m_PrimaryCommandBuffer->Flush();
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

	renderDevice->CreateRenderTarget(m_StaticFlag);
	renderDevice->CreateRenderTarget(m_VoxelAlbedo);
	renderDevice->CreateRenderTarget(m_VoxelNormal);
	renderDevice->CreateRenderTarget(m_VoxelEmissive);
	renderDevice->CreateRenderTarget(m_VoxelRadiance);

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

	m_VoxelDrawPipelines.resize(renderDevice->GetNumFramesInFlight());
	m_CommandBuffers.resize(renderDevice->GetNumFramesInFlight());

	for (size_t frameIndex = 0; frameIndex < m_VoxelDrawPipelines.size(); ++frameIndex)
	{
		renderDevice->CreatePipeline(m_VoxelDrawPipelines[frameIndex]);
		renderDevice->CreateCommandBuffer(m_CommandBuffers[frameIndex]);
		m_CommandBuffers[frameIndex]->Init(m_CommandPool, CBL_SECONDARY);
	}

	renderDevice->CreateComputePipeline(m_InjectRadiancePipeline);
	renderDevice->CreateComputePipeline(m_InjectPropagationPipeline);

	renderDevice->CreateComputePipeline(m_MipmapBasePipeline);
	renderDevice->CreateComputePipeline(m_MipmapVolumePipeline);

	SetupVoxelVolumes(dimension);
	SetupVoxelDrawPipeline();
	SetupRadiancePipeline();
	SetupMipmapPipeline();

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

	SAFE_UNINIT_CONTAINER(m_VoxelDrawPipelines);

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