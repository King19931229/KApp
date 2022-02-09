#include "KVoxilzer.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "KBase/Interface/IKLog.h"

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
	, m_Camera(nullptr)
	, m_Width(0)
	, m_Height(0)
	, m_StaticFlag(nullptr)
	, m_VoxelAlbedo(nullptr)
	, m_VoxelNormal(nullptr)
	, m_VoxelEmissive(nullptr)
	, m_VoxelRadiance(nullptr)
	, m_VolumeDimension(64)
	, m_VoxelCount(0)
	, m_NumMipmap(1)
	, m_OctreeLevel(10)
	, m_OctreeNonLeafCount(0)
	, m_OctreeLeafCount(0)
	, m_VolumeGridSize(0)
	, m_VoxelSize(0)
	, m_InjectFirstBounce(true)
	, m_VoxelDrawEnable(true)
	, m_VoxelDrawWireFrame(true)
	, m_VoxelDebugUpdate(false)
	, m_VoxelNeedUpdate(false)
	, m_VoxelUseOctree(true)
	, m_VoxelLastUseOctree(true)
{
	m_OnSceneChangedFunc = std::bind(&KVoxilzer::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KVoxilzer::~KVoxilzer()
{
}

void KVoxilzer::UpdateInternal()
{
	UpdateProjectionMatrices();

	if (m_VoxelUseOctree)
	{
		m_PrimaryCommandBuffer->BeginPrimary();

		VoxelizeStaticSceneCounter(m_PrimaryCommandBuffer, true);

		m_PrimaryCommandBuffer->End();
		m_PrimaryCommandBuffer->Flush();

		m_PrimaryCommandBuffer->BeginPrimary();

		VoxelizeStaticSceneCounter(m_PrimaryCommandBuffer, false);

		m_PrimaryCommandBuffer->End();
		m_PrimaryCommandBuffer->Flush();

		// CheckFragmentlistData();

		m_PrimaryCommandBuffer->BeginPrimary();

		BuildOctree(m_PrimaryCommandBuffer);

		m_PrimaryCommandBuffer->End();
		m_PrimaryCommandBuffer->Flush();

		uint32_t lastBuildinfo[] = { 0, 0 };
		m_BuildInfoBuffer->Read(lastBuildinfo);
		m_OctreeNonLeafCount = lastBuildinfo[0] + lastBuildinfo[1];
		m_CounterBuffer->Read(&m_OctreeLeafCount);
	}
	else
	{
		m_PrimaryCommandBuffer->BeginPrimary();

		VoxelizeStaticScene(m_PrimaryCommandBuffer);

		m_PrimaryCommandBuffer->End();
		m_PrimaryCommandBuffer->Flush();
	}

	m_PrimaryCommandBuffer->BeginPrimary();

	UpdateRadiance(m_PrimaryCommandBuffer);

	m_PrimaryCommandBuffer->End();
	m_PrimaryCommandBuffer->Flush();

	if (m_VoxelUseOctree)
	{
		ShrinkOctree();
		CheckOctreeData();
	}

}

void KVoxilzer::OnSceneChanged(EntitySceneOp op, IKEntityPtr entity)
{
	// TODO
	if (!entity->HasComponent(CT_RENDER))
	{
		return;
	}
	m_VoxelNeedUpdate = true;
}

void KVoxilzer::UpdateVoxel()
{
	if (m_VoxelLastUseOctree != m_VoxelUseOctree)
	{
		KRenderGlobal::RenderDevice->Wait();
		SetupVoxelReleatedData();
	}

	if (m_VoxelDebugUpdate || m_VoxelNeedUpdate || m_VoxelLastUseOctree != m_VoxelUseOctree)
	{
		UpdateInternal();
		m_VoxelNeedUpdate = false;
		m_VoxelLastUseOctree = m_VoxelUseOctree;
	}
}

void KVoxilzer::ReloadShader()
{
	m_VoxelDrawVS->Reload();
	m_VoxelDrawOctreeVS->Reload();
	m_VoxelDrawGS->Reload();
	m_VoxelWireFrameDrawGS->Reload();
	m_VoxelDrawFS->Reload();
	m_QuadVS->Reload();
	m_LightPassFS->Reload();
	m_LightPassOctreeFS->Reload();
	m_OctreeRayTestFS->Reload();

	if (m_VoxelUseOctree)
	{
		m_OctreeTagNodePipeline->ReloadShader();
		m_OctreeInitNodePipeline->ReloadShader();
		m_OctreeAllocNodePipeline->ReloadShader();
		m_OctreeModifyArgPipeline->ReloadShader();

		m_OctreeInitDataPipeline->ReloadShader();
		m_OctreeAssignDataPipeline->ReloadShader();

		m_InjectRadianceOctreePipeline->ReloadShader();
		m_InjectPropagationOctreePipeline->ReloadShader();
		m_MipmapBaseOctreePipeline->ReloadShader();

		for (IKPipelinePtr& pipeline : m_VoxelDrawOctreePipelines)
		{
			pipeline->Reload();
		}
		for (IKPipelinePtr& pipeline : m_VoxelWireFrameDrawOctreePipelines)
		{
			pipeline->Reload();
		}
		for (IKPipelinePtr& pipeline : m_LightPassOctreePipelines)
		{
			pipeline->Reload();
		}
		for (IKPipelinePtr& pipeline : m_OctreeRayTestPipelines)
		{
			pipeline->Reload();
		}
	}
	else
	{
		m_ClearDynamicPipeline->ReloadShader();
		m_InjectRadiancePipeline->ReloadShader();
		m_InjectPropagationPipeline->ReloadShader();

		for (IKPipelinePtr& pipeline : m_VoxelDrawPipelines)
		{
			pipeline->Reload();
		}
		for (IKPipelinePtr& pipeline : m_VoxelWireFrameDrawPipelines)
		{
			pipeline->Reload();
		}
		for (IKPipelinePtr& pipeline : m_LightPassPipelines)
		{
			pipeline->Reload();
		}
	}

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

	m_VolumeCenter = center;
	m_VolumeMin = min;
	m_VolumeMax = max;

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
			if (detail.semantic == CS_VOXEL_MISCS)
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

void KVoxilzer::SetupVoxelBuffer()
{
	uint32_t dimension = m_VolumeDimension;
	uint32_t baseMipmapDimension = (dimension + 1) / 2;

	m_VoxelAlbedo->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	m_VoxelNormal->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	m_VoxelRadiance->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	m_VoxelEmissive->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8GB8BA8_UNORM);
	m_StaticFlag->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8_UNORM);
}

void KVoxilzer::SetupSparseVoxelBuffer()
{
	uint32_t counter = 0;
	uint32_t countOnly = 1;

	m_CounterBuffer->InitMemory(sizeof(counter), &counter);
	m_CounterBuffer->InitDevice(false);

	uint32_t fragmentDummy[] = { 0, 0, 0, 0 };
	m_FragmentlistBuffer->InitMemory(sizeof(fragmentDummy), fragmentDummy);
	m_FragmentlistBuffer->InitDevice(false);

	m_CountOnlyBuffer->InitMemory(sizeof(countOnly), &countOnly);
	m_CountOnlyBuffer->InitDevice(false);

	uint32_t buildinfo[] = { 0, 8 };
	m_BuildInfoBuffer->InitMemory(sizeof(buildinfo), buildinfo);
	m_BuildInfoBuffer->InitDevice(false);

	uint32_t indirectinfo[] = { 1, 1, 1 };
	m_BuildIndirectBuffer->InitMemory(sizeof(indirectinfo), indirectinfo);
	m_BuildIndirectBuffer->InitDevice(true);

	// 随意填充空数据即可
	m_OctreeBuffer->InitMemory(sizeof(fragmentDummy), fragmentDummy);
	m_OctreeBuffer->InitDevice(false);

	m_OctreeDataBuffer->InitMemory(sizeof(fragmentDummy), fragmentDummy);
	m_OctreeDataBuffer->InitDevice(false);

	m_OctreeMipmapDataBuffer->InitMemory(sizeof(fragmentDummy), fragmentDummy);
	m_OctreeMipmapDataBuffer->InitDevice(false);

	glm::vec4 cameraDummy[5] = { glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, -1, 0), glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 0, 0) };
	for (IKStorageBufferPtr buff : m_OctreeCameraBuffers)
	{
		buff->InitMemory(sizeof(cameraDummy), cameraDummy);
		buff->InitDevice(false);
	}
}

void KVoxilzer::SetupVoxelReleatedData()
{
	uint32_t dimension = m_VolumeDimension;
	uint32_t baseMipmapDimension = (dimension + 1) / 2;

	m_VoxelRenderPassTarget->InitFromColor(dimension, dimension, 1, EF_R8_UNORM);
	m_VoxelRenderPass->UnInit();
	m_VoxelRenderPass->SetColorAttachment(0, m_VoxelRenderPassTarget->GetFrameBuffer());
	m_VoxelRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_VoxelRenderPass->Init();

	m_LightPassTarget->InitFromColor(m_Width, m_Height, 1, EF_R8GB8BA8_UNORM);
	m_LightPassRenderPass->UnInit();
	m_LightPassRenderPass->SetColorAttachment(0, m_LightPassTarget->GetFrameBuffer());
	m_LightPassRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_LightPassRenderPass->Init();

	if (m_VoxelUseOctree)
	{
		m_VoxelAlbedo->UnInit();
		m_VoxelNormal->UnInit();
		m_VoxelRadiance->UnInit();
		m_VoxelEmissive->UnInit();
		m_StaticFlag->UnInit();
		SetupSparseVoxelBuffer();
		SetupOctreeBuildPipeline();
	}
	else
	{
		m_CounterBuffer->UnInit();
		m_FragmentlistBuffer->UnInit();
		m_CountOnlyBuffer->UnInit();
		m_BuildInfoBuffer->UnInit();
		m_BuildIndirectBuffer->UnInit();
		m_OctreeBuffer->UnInit();
		m_OctreeDataBuffer->UnInit();
		for (IKStorageBufferPtr buff : m_OctreeCameraBuffers)
		{
			buff->UnInit();
		}
		SetupVoxelBuffer();
	}

	for (uint32_t i = 0; i < 6; ++i)
	{
		m_VoxelTexMipmap[i]->InitFromStorage3D(baseMipmapDimension, baseMipmapDimension, baseMipmapDimension, m_NumMipmap, EF_R8GB8BA8_UNORM);
	}

	SetupVoxelDrawPipeline();
	SetupClearDynamicPipeline();
	SetupRadiancePipeline();
	SetupMipmapPipeline();
	SetupOctreeMipmapPipeline();
	SetupLightPassPipeline(m_Width, m_Height);

	SetupOctreeBuildPipeline();
	SetupRayTestPipeline(m_Width, m_Height);
}

void KVoxilzer::SetupVoxelVolumes(uint32_t dimension)
{
	m_VolumeDimension = dimension;
	m_VoxelCount = m_VolumeDimension * m_VolumeDimension * m_VolumeDimension;
	m_VoxelSize = m_VolumeGridSize / m_VolumeDimension;

	uint32_t baseMipmapDimension = (dimension + 1) / 2;
	m_NumMipmap = (uint32_t)(std::floor(std::log(baseMipmapDimension) / std::log(2)) + 1);

	m_OctreeLevel = m_NumMipmap;

	m_CloestSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_CloestSampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	m_CloestSampler->Init(0, 0);

	m_LinearSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_LinearSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_LinearSampler->Init(0, 0);

	m_MipmapSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_MipmapSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_MipmapSampler->Init(0, m_NumMipmap - 1);

	UpdateProjectionMatrices();

	SetupVoxelReleatedData();
}

void KVoxilzer::SetupVoxelDrawPipeline()
{
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/draw_voxels.vert", m_VoxelDrawVS, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/draw_voxels_octree.vert", m_VoxelDrawOctreeVS, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/draw_voxels.geom", m_VoxelDrawGS, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/draw_voxels_wireframe.geom", m_VoxelWireFrameDrawGS, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/draw_voxels.frag", m_VoxelDrawFS, true));

	enum
	{
		DEFAULT,
		WIREFRAME,
		COUNT
	};

	for (size_t idx = 0; idx < COUNT; ++idx)
	{
		std::vector<IKPipelinePtr>* pipelines = nullptr;

		if (m_VoxelUseOctree)
		{
			pipelines = idx == DEFAULT ? &m_VoxelDrawOctreePipelines : &m_VoxelWireFrameDrawOctreePipelines;
		}
		else
		{
			pipelines = idx == DEFAULT ? &m_VoxelDrawPipelines : &m_VoxelWireFrameDrawPipelines;
		}

		for (size_t frameIndex = 0; frameIndex < pipelines->size(); ++frameIndex)
		{
			IKPipelinePtr& pipeline = (*pipelines)[frameIndex];

			pipeline->SetShader(ST_VERTEX, m_VoxelUseOctree ? m_VoxelDrawOctreeVS :	m_VoxelDrawVS);

			pipeline->SetPrimitiveTopology(PT_POINT_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_NONE);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetShader(ST_GEOMETRY, idx == 0 ? m_VoxelDrawGS : m_VoxelWireFrameDrawGS);
			pipeline->SetShader(ST_FRAGMENT, m_VoxelDrawFS);

			IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_VOXEL);
			pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelBuffer);

			IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
			pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, cameraBuffer);

			if (m_VoxelUseOctree)
			{
				pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE, ST_VERTEX, m_OctreeBuffer);
				pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE_DATA, ST_VERTEX, m_OctreeDataBuffer);
			}
			else
			{
				pipeline->SetStorageImage(VOXEL_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), EF_UNKNOWN);
				pipeline->SetStorageImage(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), EF_UNKNOWN);
				pipeline->SetStorageImage(VOXEL_BINDING_EMISSION, m_VoxelEmissive->GetFrameBuffer(), EF_UNKNOWN);
				pipeline->SetStorageImage(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN);
				//
				pipeline->SetStorageImage(VOXEL_BINDING_TEXMIPMAP_OUT, m_VoxelTexMipmap[0]->GetFrameBuffer(), EF_UNKNOWN);
			}

			pipeline->Init();
		}
	}

	m_VoxelDrawVertexData.vertexCount = (uint32_t)(m_VolumeDimension * m_VolumeDimension * m_VolumeDimension);
	m_VoxelDrawVertexData.vertexStart = 0;
}

void KVoxilzer::SetupClearDynamicPipeline()
{
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_VOXEL);

	if (m_VoxelUseOctree)
	{
		m_ClearDynamicPipeline->UnInit();
	}
	else
	{
		m_ClearDynamicPipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);

		m_ClearDynamicPipeline->BindStorageImage(VOXEL_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, false);
		m_ClearDynamicPipeline->BindStorageImage(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
		m_ClearDynamicPipeline->BindStorageImage(VOXEL_BINDING_EMISSION, m_VoxelEmissive->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
		m_ClearDynamicPipeline->BindStorageImage(VOXEL_BINDING_STATIC_FLAG, m_StaticFlag->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, false);

		m_ClearDynamicPipeline->Init("voxel/clear_dynamic.comp");
	}
}

void KVoxilzer::SetupRadiancePipeline()
{
	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_GLOBAL);
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_VOXEL);

	// Inject Radiance
	IKComputePipelinePtr& injectRadiancePipeline = m_VoxelUseOctree ? m_InjectRadianceOctreePipeline : m_InjectRadiancePipeline;

	injectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_GLOBAL, globalBuffer);
	injectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);

	if (m_VoxelUseOctree)
	{
		injectRadiancePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		injectRadiancePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		injectRadiancePipeline->Init("voxel/inject_radiance_octree.comp");
	}
	else
	{
		injectRadiancePipeline->BindSampler(VOXEL_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), m_LinearSampler, true);
		injectRadiancePipeline->BindStorageImage(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
		injectRadiancePipeline->BindStorageImage(VOXEL_BINDING_EMISSION_MAP, m_VoxelEmissive->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);
		injectRadiancePipeline->BindStorageImage(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
		injectRadiancePipeline->Init("voxel/inject_radiance.comp");
	}

	// Inject Propagation
	IKComputePipelinePtr& injectPropagationPipeline = m_VoxelUseOctree ? m_InjectPropagationOctreePipeline : m_InjectPropagationPipeline;

	injectPropagationPipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);

	if (m_VoxelUseOctree)
	{
		injectPropagationPipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		injectPropagationPipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	}
	else
	{
		injectPropagationPipeline->BindStorageImage(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
		injectPropagationPipeline->BindSampler(VOXEL_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), m_LinearSampler, false);
		injectPropagationPipeline->BindSampler(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), m_LinearSampler, false);
	}

	std::vector<IKFrameBufferPtr> targets;
	std::vector<IKSamplerPtr> samplers;
	targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	samplers.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
	{
		targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
		samplers[i] = m_MipmapSampler;
	}

	injectPropagationPipeline->BindSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers, false);

	if (m_VoxelUseOctree)
	{
		injectPropagationPipeline->Init("voxel/inject_propagation_octree.comp");
	}
	else
	{
		injectPropagationPipeline->Init("voxel/inject_propagation.comp");
	}
}

void KVoxilzer::SetupMipmapPipeline()
{
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_VOXEL);

	IKComputePipelinePtr& mipmapBasePipeline = m_VoxelUseOctree ? m_MipmapBaseOctreePipeline : m_MipmapBasePipeline;

	std::vector<IKFrameBufferPtr> targets;
	std::vector<IKSamplerPtr> samplers;
	targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	samplers.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
	{
		targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
		samplers[i] = m_MipmapSampler;
	}

	mipmapBasePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
	mipmapBasePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	mipmapBasePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);

	if (m_VoxelUseOctree)
	{
		mipmapBasePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		mipmapBasePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		mipmapBasePipeline->Init("voxel/aniso_mipmapbase_octree.comp");
	}
	else
	{
		mipmapBasePipeline->BindSampler(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), m_LinearSampler, false);
		mipmapBasePipeline->Init("voxel/aniso_mipmapbase.comp");
	}

	m_MipmapVolumePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
	m_MipmapVolumePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_MipmapVolumePipeline->BindSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers, false);
	m_MipmapVolumePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);

	m_MipmapVolumePipeline->Init("voxel/aniso_mipmapvolume.comp");
}

void KVoxilzer::SetupOctreeMipmapPipeline()
{
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(0, CBT_VOXEL);

	m_OctreeMipmapBasePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
	m_OctreeMipmapBasePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_OctreeMipmapBasePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapBasePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapBasePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_MIPMAP_DATA, m_OctreeMipmapDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapBasePipeline->Init("voxel/aniso_octree_mipmapbase.comp");

	m_OctreeMipmapVolumePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
	m_OctreeMipmapVolumePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_OctreeMipmapVolumePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapVolumePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_MIPMAP_DATA, m_OctreeMipmapDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapVolumePipeline->Init("voxel/aniso_octree_mipmapvolume.comp");
}

void KVoxilzer::Resize(uint32_t width, uint32_t height)
{
	m_LightPassTarget->UnInit();
	m_LightPassRenderPass->UnInit();

	m_LightPassTarget->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM);
	m_LightPassRenderPass->SetColorAttachment(0, m_LightPassTarget->GetFrameBuffer());
	m_LightPassRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_LightPassRenderPass->Init();

	m_OctreeRayTestTarget->UnInit();
	m_OctreeRayTestPass->UnInit();

	m_OctreeRayTestTarget->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM);
	m_OctreeRayTestPass->SetColorAttachment(0, m_OctreeRayTestTarget->GetFrameBuffer());
	m_OctreeRayTestPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_OctreeRayTestPass->Init();

	// 清空RenderTarget同时Translate Layout到Shader可读
	m_PrimaryCommandBuffer->BeginPrimary();

	m_PrimaryCommandBuffer->BeginRenderPass(m_LightPassRenderPass, SUBPASS_CONTENTS_INLINE);
	m_PrimaryCommandBuffer->EndRenderPass();

	m_PrimaryCommandBuffer->BeginRenderPass(m_OctreeRayTestPass, SUBPASS_CONTENTS_INLINE);
	m_PrimaryCommandBuffer->EndRenderPass();

	m_PrimaryCommandBuffer->End();
	m_PrimaryCommandBuffer->Flush();
}

void KVoxilzer::SetupQuadDrawData()
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
}

void KVoxilzer::SetupLightPassPipeline(uint32_t width, uint32_t height)
{
	m_LightDebugDrawer.Init(m_LightPassTarget, 0, 0, 1, 1);

	std::vector<IKFrameBufferPtr> targets;
	std::vector<IKSamplerPtr> samplers;
	targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	samplers.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
	{
		targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
		samplers[i] = m_MipmapSampler;
	}

	std::vector<IKPipelinePtr>& pipelines = m_VoxelUseOctree ? m_LightPassOctreePipelines : m_LightPassPipelines;

	for (size_t frameIndex = 0; frameIndex < pipelines.size(); ++frameIndex)
	{
		IKPipelinePtr& pipeline = pipelines[frameIndex];

		pipeline->SetVertexBinding(ms_QuadFormats, ARRAY_SIZE(ms_QuadFormats));
		pipeline->SetShader(ST_VERTEX, m_QuadVS);

		if (m_VoxelUseOctree)
		{
			pipeline->SetShader(ST_FRAGMENT, m_LightPassOctreeFS);
		}
		else
		{
			pipeline->SetShader(ST_FRAGMENT, m_LightPassFS);
		}

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
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
			true);
		pipeline->SetSampler(VOXEL_BINDING_GBUFFER_POSITION,
			KRenderGlobal::GBuffer.GetGBufferTarget(KGBuffer::RT_POSITION)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
		pipeline->SetSampler(VOXEL_BINDING_GBUFFER_ALBEDO,
			KRenderGlobal::GBuffer.GetGBufferTarget(KGBuffer::RT_DIFFUSE)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
		pipeline->SetSampler(VOXEL_BINDING_GBUFFER_SPECULAR,
			KRenderGlobal::GBuffer.GetGBufferTarget(KGBuffer::RT_SPECULAR)->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		if (m_VoxelUseOctree)
		{
			pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE, ST_FRAGMENT, m_OctreeBuffer);
			pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE_DATA, ST_VERTEX, m_OctreeDataBuffer);
		}
		else
		{
			pipeline->SetSampler(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), m_LinearSampler, false);
			pipeline->SetSampler(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), m_LinearSampler, false);
		}

		pipeline->SetSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers, false);

		pipeline->Init();
	}
}

void KVoxilzer::SetupRayTestPipeline(uint32_t width, uint32_t height)
{
	m_OctreeRayTestDebugDrawer.Init(m_OctreeRayTestTarget, 0, 0, 1, 1);

	if (m_VoxelUseOctree)
	{
		for (size_t frameIndex = 0; frameIndex < m_OctreeRayTestPipelines.size(); ++frameIndex)
		{
			IKPipelinePtr& pipeline = m_OctreeRayTestPipelines[frameIndex];

			pipeline->SetVertexBinding(ms_QuadFormats, ARRAY_SIZE(ms_QuadFormats));
			pipeline->SetShader(ST_VERTEX, m_QuadVS);
			pipeline->SetShader(ST_FRAGMENT, m_OctreeRayTestFS);

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_NONE);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetStorageBuffer(OCTREE_BINDING_OCTREE, ST_FRAGMENT, m_OctreeBuffer);
			pipeline->SetStorageBuffer(OCTREE_BINDING_OCTREE_DATA, ST_FRAGMENT, m_OctreeDataBuffer);
			pipeline->SetStorageBuffer(OCTREE_BINDING_CAMERA, ST_FRAGMENT, m_OctreeCameraBuffers[frameIndex]);

			pipeline->Init();
		}
	}
	else
	{
		for (size_t frameIndex = 0; frameIndex < m_OctreeRayTestPipelines.size(); ++frameIndex)
		{
			m_OctreeRayTestPipelines[frameIndex]->UnInit();
		}
	}
}

void KVoxilzer::ClearDynamicScene(IKCommandBufferPtr commandBuffer)
{
	uint32_t group = (m_VolumeDimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;
	m_ClearDynamicPipeline->Execute(commandBuffer, group, group, group, 0);
}

void KVoxilzer::VoxelizeStaticScene(IKCommandBufferPtr commandBuffer)
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	// TODO
	std::vector<KRenderComponent*> cullRes;
	((KRenderScene*)m_Scene)->GetRenderComponent(sceneBox, false, cullRes);

	if (cullRes.size() == 0) return;

	commandBuffer->BeginRenderPass(m_VoxelRenderPass, SUBPASS_CONTENTS_INLINE);
	commandBuffer->SetViewport(m_VoxelRenderPass->GetViewPort());

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

				command.pipeline->GetHandle(m_VoxelRenderPass, command.pipelineHandle);

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

void KVoxilzer::VoxelizeStaticSceneCounter(IKCommandBufferPtr commandBuffer, bool bCountOnly)
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	std::vector<KRenderComponent*> cullRes;
	((KRenderScene*)m_Scene)->GetRenderComponent(sceneBox, false, cullRes);

	uint32_t counter = 0;
	uint32_t countOnly = bCountOnly;

	if (!bCountOnly)
	{
		m_CounterBuffer->Read(&counter);
		if (counter == 0)
		{
			return;
		}

		uint32_t bufferSize = counter * sizeof(glm::uvec4);
		if (m_FragmentlistBuffer->GetBufferSize() != bufferSize)
		{
			m_FragmentlistBuffer->UnInit();
			m_FragmentlistBuffer->InitMemory(bufferSize, nullptr);
			m_FragmentlistBuffer->InitDevice(false);
		}

		counter = 0;
	}

	m_CounterBuffer->Write(&counter);
	m_CountOnlyBuffer->Write(&countOnly);

	if (cullRes.size() == 0) return;

	commandBuffer->BeginRenderPass(m_VoxelRenderPass, SUBPASS_CONTENTS_INLINE);
	commandBuffer->SetViewport(m_VoxelRenderPass->GetViewPort());

	std::vector<KRenderCommand> commands;
	for (KRenderComponent* render : cullRes)
	{
		render->Visit(PIPELINE_STAGE_SPARSE_VOXEL, 0, [&](KRenderCommand& command)
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

				command.pipeline->GetHandle(m_VoxelRenderPass, command.pipelineHandle);

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

void KVoxilzer::CheckFragmentlistData()
{
	uint32_t counter = 0;
	m_CounterBuffer->Read(&counter);

	std::vector<glm::uvec4> fragments;
	std::vector<glm::uvec3> positions;
	fragments.resize(counter);
	positions.resize(counter);

	m_FragmentlistBuffer->Read(fragments.data());
	for (size_t i = 0; i < fragments.size(); ++i)
	{
		const glm::uvec4& ufragment = fragments[i];
		glm::uvec3 pos = glm::uvec3(ufragment.x & 0xfffu, (ufragment.x >> 12u) & 0xfffu, (ufragment.x >> 24u) | ((ufragment.y >> 28u) << 8u));
		positions[i] = pos;
	}

	KG_LOG(LM_DEFAULT, "Voxel fragments positions:");
	for (size_t i = 0; i < positions.size(); ++i)
	{
		const glm::uvec3& pos = positions[i];
		KG_LOG(LM_DEFAULT, "\t%d %d %d\n", pos[0], pos[1], pos[2]);
	}
}

void KVoxilzer::CheckOctreeData()
{
	static_assert(sizeof(glm::uvec4) == OCTREE_DATA_SIZE, "Check data size");
	std::vector<glm::uvec4> datas;
	datas.resize(m_OctreeDataBuffer->GetBufferSize() / OCTREE_DATA_SIZE);

	m_OctreeDataBuffer->Read(datas.data());

	for (size_t i = 0; i < datas.size(); ++i)
	{
		if (datas[i].y != 0)
		{
			uint32_t encoded = datas[i].w;
			glm::uvec4 unpacked = glm::uvec4(encoded & 0xff, (encoded >> 8) & 0xff, (encoded >> 16) & 0xff, (encoded >> 24) & 0xff);
			uint32_t count = unpacked.w & 0x3fu;
			glm::vec3 data = glm::vec3(unpacked.x / 255.0f, unpacked.y / 255.0f, unpacked.z / 255.0f);
			float luminance = glm::dot(data, glm::vec3(0.2126f, 0.7152f, 0.0722f));
			if (count && luminance == 0.0f)
			{
				// KG_LOG(LM_DEFAULT, "Voxel radiance miss");
			}
		}
	}
	
	struct OctreeMipmapData
	{
		uint32_t data[6];
	};
	static_assert(sizeof(OctreeMipmapData) == OCTREE_MIPMAP_DATA_SIZE, "Check data size");
	std::vector<OctreeMipmapData> mipmapDatas;
	mipmapDatas.resize(m_OctreeMipmapDataBuffer->GetBufferSize() / OCTREE_MIPMAP_DATA_SIZE);

	m_OctreeMipmapDataBuffer->Read(mipmapDatas.data());

	for (size_t i = 0; i < mipmapDatas.size(); ++i)
	{
		uint32_t* encoded = mipmapDatas[i].data;
		for (size_t j = 0; j < 6; ++j)
		{
			glm::uvec4 unpacked = glm::uvec4(encoded[j] & 0xff, (encoded[j] >> 8) & 0xff, (encoded[j] >> 16) & 0xff, (encoded[j] >> 24) & 0xff);
			glm::vec4 data = glm::vec4(unpacked) / 255.0f;
			float luminance = glm::dot(data, glm::vec4(0.2126f, 0.7152f, 0.0722f, 0.0f));
			if (luminance == 0.0f)
			{
				// KG_LOG(LM_DEFAULT, "Miapmap voxel radiance miss");
			}
		}
	}
}

inline static constexpr uint32_t group_x_64(uint32_t x) { return (x >> 6u) + ((x & 0x3fu) ? 1u : 0u); }

void KVoxilzer::BuildOctree(IKCommandBufferPtr commandBuffer)
{
	uint32_t fragmentCount = 0;
	m_CounterBuffer->Read(&fragmentCount);

	// Estimate octree buffer size
	uint32_t octreeNodeNum = std::max((uint32_t)OCTREE_NODE_NUM_MIN, fragmentCount << 2u);
	octreeNodeNum = std::min(octreeNodeNum, (uint32_t)OCTREE_NODE_NUM_MAX);

	uint32_t preOctreeNodeNum = (uint32_t)m_OctreeBuffer->GetBufferSize() / OCTREE_NODE_SIZE;
	if (octreeNodeNum > preOctreeNodeNum)
	{
		m_OctreeBuffer->UnInit();
		m_OctreeBuffer->InitMemory(octreeNodeNum * OCTREE_NODE_SIZE, nullptr);
		m_OctreeBuffer->InitDevice(false);

		m_OctreeMipmapDataBuffer->UnInit();
		m_OctreeMipmapDataBuffer->InitMemory(octreeNodeNum * OCTREE_MIPMAP_DATA_SIZE, nullptr);
		m_OctreeMipmapDataBuffer->InitDevice(false);
	}

	m_OctreeDataBuffer->UnInit();
	m_OctreeDataBuffer->InitMemory(fragmentCount * OCTREE_DATA_SIZE, nullptr);
	m_OctreeDataBuffer->InitDevice(false);

	uint32_t buildinfo[] = { 0, 8 };
	m_BuildInfoBuffer->InitMemory(sizeof(buildinfo), buildinfo);
	m_BuildInfoBuffer->InitDevice(false);

	uint32_t indirectinfo[] = { 1, 1, 1 };
	m_BuildIndirectBuffer->InitMemory(sizeof(indirectinfo), indirectinfo);
	m_BuildIndirectBuffer->InitDevice(true);

	uint32_t fragmentGroupX = group_x_64(fragmentCount);

	glm::uvec2 constant = glm::uvec2(fragmentCount, m_VolumeDimension);
	KDynamicConstantBufferUsage usage;
	usage.binding = OCTREE_BINDING_OBJECT;
	usage.range = sizeof(constant);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

	fragmentCount = 0;
	m_CounterBuffer->Write(&fragmentCount);

	for (uint32_t i = 1; i <= m_OctreeLevel; ++i)
	{
		m_OctreeInitNodePipeline->ExecuteIndirect(commandBuffer, m_BuildIndirectBuffer, 0, nullptr);
		m_OctreeTagNodePipeline->Execute(commandBuffer, fragmentGroupX, 1, 1, 0, &usage);
		if (i != m_OctreeLevel)
		{
			m_OctreeAllocNodePipeline->ExecuteIndirect(commandBuffer, m_BuildIndirectBuffer, 0, nullptr);
			m_OctreeModifyArgPipeline->Execute(commandBuffer, 1, 1, 1, 0, nullptr);
		}
		else
		{
			uint32_t counter = 0;
			m_CounterBuffer->Write(&counter);
			m_OctreeInitDataPipeline->ExecuteIndirect(commandBuffer, m_BuildIndirectBuffer, 0, nullptr);
			m_OctreeAssignDataPipeline->Execute(commandBuffer, fragmentGroupX, 1, 1, 0, &usage);
		}
	}
}

void KVoxilzer::ShrinkOctree()
{
	std::vector<char> buffers;

	buffers.resize(m_OctreeBuffer->GetBufferSize());
	m_OctreeBuffer->Read(buffers.data());
	m_OctreeBuffer->UnInit();
	m_OctreeBuffer->InitMemory(m_OctreeNonLeafCount * OCTREE_NODE_SIZE, buffers.data());
	m_OctreeBuffer->InitDevice(false);

	buffers.resize(m_OctreeMipmapDataBuffer->GetBufferSize());
	m_OctreeMipmapDataBuffer->Read(buffers.data());
	m_OctreeMipmapDataBuffer->UnInit();
	m_OctreeMipmapDataBuffer->InitMemory(m_OctreeNonLeafCount * OCTREE_MIPMAP_DATA_SIZE, buffers.data());
	m_OctreeMipmapDataBuffer->InitDevice(false);

	buffers.resize(m_OctreeDataBuffer->GetBufferSize());
	m_OctreeDataBuffer->Read(buffers.data());
	m_OctreeDataBuffer->UnInit();
	m_OctreeDataBuffer->InitMemory(m_OctreeLeafCount * OCTREE_DATA_SIZE, buffers.data());
	m_OctreeDataBuffer->InitDevice(false);
}

void KVoxilzer::UpdateRadiance(IKCommandBufferPtr commandBuffer)
{
	InjectRadiance(commandBuffer);
	GenerateMipmap(commandBuffer);

	if (m_InjectFirstBounce)
	{
		uint32_t group = (m_VolumeDimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;
		IKComputePipelinePtr& propagationPipeline = m_VoxelUseOctree ? m_InjectPropagationOctreePipeline : m_InjectPropagationPipeline;
		propagationPipeline->Execute(commandBuffer, group, group, group, 0);
		GenerateMipmap(commandBuffer);
	}
}

void KVoxilzer::InjectRadiance(IKCommandBufferPtr commandBuffer)
{
	uint32_t group = (m_VolumeDimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;
	IKComputePipelinePtr& injectPipeline = m_VoxelUseOctree ? m_InjectRadianceOctreePipeline : m_InjectRadiancePipeline;
	injectPipeline->Execute(commandBuffer, group, group, group, 0);
}

void KVoxilzer::GenerateMipmap(IKCommandBufferPtr commandBuffer)
{
	GenerateMipmapBase(commandBuffer);
	GenerateMipmapVolume(commandBuffer);

	if (m_VoxelUseOctree)
	{
		GenerateOctreeMipmapBase(commandBuffer);
		GenerateOctreeMipmapVolume(commandBuffer);
	}
}

void KVoxilzer::GenerateMipmapBase(IKCommandBufferPtr commandBuffer)
{
	uint32_t dimension = m_VolumeDimension / 2;
	uint32_t group = (dimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;

	glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, 0);

	KDynamicConstantBufferUsage usage;
	usage.binding = SHADER_BINDING_OBJECT;
	usage.range = sizeof(constant);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

	IKComputePipelinePtr& mipmapBasePipeline = m_VoxelUseOctree ? m_MipmapBaseOctreePipeline : m_MipmapBasePipeline;
	mipmapBasePipeline->Execute(commandBuffer, group, group, group, 0, &usage);
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
		uint32_t group = (dimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;

		for (size_t idx = 0; idx < ARRAY_SIZE(m_VoxelTexMipmap); ++idx)
		{
			glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, mipmap);

			KDynamicConstantBufferUsage usage;
			usage.binding = SHADER_BINDING_OBJECT;
			usage.range = sizeof(constant);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

			m_MipmapVolumePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, EF_UNKNOWN, COMPUTE_RESOURCE_OUT, mipmap, true);
			m_MipmapVolumePipeline->Execute(commandBuffer, group, group, group, 0, &usage);
		}

		dimension /= 2;
		++mipmap;
	}

	ASSERT_RESULT(mipmap == m_NumMipmap);
}

void KVoxilzer::GenerateOctreeMipmapBase(IKCommandBufferPtr commandBuffer)
{
	uint32_t dimension = m_VolumeDimension / 2;
	uint32_t group = (dimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;

	glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, 0);

	KDynamicConstantBufferUsage usage;
	usage.binding = SHADER_BINDING_OBJECT;
	usage.range = sizeof(constant);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

	m_OctreeMipmapBasePipeline->Execute(commandBuffer, group, group, group, 0, &usage);
}

void KVoxilzer::GenerateOctreeMipmapVolume(IKCommandBufferPtr commandBuffer)
{
	uint32_t dimension = m_VolumeDimension / 4;
	uint32_t mipmap = 1;

	while (dimension > 0)
	{
		uint32_t group = (dimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;

		glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, mipmap);

		KDynamicConstantBufferUsage usage;
		usage.binding = SHADER_BINDING_OBJECT;
		usage.range = sizeof(constant);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

		m_OctreeMipmapVolumePipeline->Execute(commandBuffer, group, group, group, 0, &usage);

		dimension /= 2;
		++mipmap;
	}

	ASSERT_RESULT(mipmap == m_NumMipmap);
}

bool KVoxilzer::EnableLightDebugDraw()
{
	m_LightDebugDrawer.EnableDraw();
	return true;
}

bool KVoxilzer::DisableLightDebugDraw()
{
	m_LightDebugDrawer.DisableDraw();
	return true;
}

bool KVoxilzer::GetLightDebugRenderCommand(KRenderCommandList& commands)
{
	return m_LightDebugDrawer.GetDebugRenderCommand(commands);
}

bool KVoxilzer::EnableOctreeRayTestDebugDraw()
{
	m_OctreeRayTestDebugDrawer.EnableDraw();
	return true;
}

bool KVoxilzer::DisableOctreeRayTestDebugDraw()
{
	m_OctreeRayTestDebugDrawer.DisableDraw();
	return true;
}

bool KVoxilzer::GetOctreeRayTestRenderCommand(KRenderCommandList& commands)
{
	return m_OctreeRayTestDebugDrawer.GetDebugRenderCommand(commands);
}

bool KVoxilzer::RenderVoxel(size_t frameIndex, IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers)
{
	if (!m_VoxelDrawEnable)
		return true;

	std::vector<IKPipelinePtr>* pipelines = nullptr;

	if (m_VoxelUseOctree)
	{
		pipelines = m_VoxelDrawWireFrame ? &m_VoxelWireFrameDrawOctreePipelines : &m_VoxelDrawOctreePipelines;
	}
	else
	{
		pipelines = m_VoxelDrawWireFrame ? &m_VoxelWireFrameDrawPipelines : &m_VoxelDrawPipelines;
	}

	if (frameIndex < pipelines->size())
	{
		KRenderCommand command;
		command.vertexData = &m_VoxelDrawVertexData;
		command.indexData = nullptr;
		command.pipeline = (*pipelines)[frameIndex];
		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = false;

		IKCommandBufferPtr commandBuffer = m_DrawCommandBuffers[frameIndex];

		commandBuffer->BeginSecondary(renderPass);
		commandBuffer->SetViewport(renderPass->GetViewPort());
		commandBuffer->Render(command);
		commandBuffer->End();

		buffers.push_back(commandBuffer);

		return true;
	}
	return false;
}

bool KVoxilzer::UpdateLightingResult(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	std::vector<IKPipelinePtr>& lightPassPipelines = m_VoxelUseOctree ? m_LightPassOctreePipelines : m_LightPassPipelines;

	if (frameIndex < lightPassPipelines.size())
	{
		IKCommandBufferPtr commandBuffer = m_LightingCommandBuffers[frameIndex];

		primaryBuffer->BeginRenderPass(m_LightPassRenderPass, SUBPASS_CONTENTS_SECONDARY);

		KRenderCommand command;
		command.vertexData = &m_QuadVertexData;
		command.indexData = &m_QuadIndexData;
		command.pipeline = lightPassPipelines[frameIndex];
		command.pipeline->GetHandle(m_LightPassRenderPass, command.pipelineHandle);
		command.indexDraw = true;

		commandBuffer->BeginSecondary(m_LightPassRenderPass);
		commandBuffer->SetViewport(m_LightPassRenderPass->GetViewPort());
		commandBuffer->Render(command);
		commandBuffer->End();

		primaryBuffer->Execute(commandBuffer);
		primaryBuffer->EndRenderPass();

		return true;
	}
	return false;
}

bool KVoxilzer::UpdateOctreRayTestResult(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	if (frameIndex < m_OctreeRayTestPipelines.size())
	{
		if (m_VoxelUseOctree && m_OctreeRayTestDebugDrawer.GetEnable())
		{
			IKCommandBufferPtr commandBuffer = m_OctreeRayTestCommandBuffers[frameIndex];

			IKStorageBufferPtr cameraBuffer = m_OctreeCameraBuffers[frameIndex];

			glm::vec4 cameraInfo[5] = {};
			cameraInfo[0] = glm::vec4(m_Camera->GetPosition(), 1.0f);
			cameraInfo[1] = glm::vec4(m_Camera->GetForward(), 0.0f);
			cameraInfo[2] = glm::vec4(m_Camera->GetRight(), 0.0f);
			cameraInfo[3] = glm::vec4(m_Camera->GetUp(), 0.0f);

			// 转换到[0,1]空间里
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), -m_VolumeMin);
			// 小心除0
			transform = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / std::max(m_VolumeGridSize, 1e-5f))) * transform;
			// 平移到[1,2]空间里
			transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f)) * transform;

			cameraInfo[0] = transform * cameraInfo[0];
			cameraInfo[1] = glm::normalize(transform * cameraInfo[1]);
			cameraInfo[2] = glm::normalize(transform * cameraInfo[2]);
			cameraInfo[3] = glm::normalize(transform * cameraInfo[3]);

			cameraInfo[4][0] = m_Camera->GetNear();
			cameraInfo[4][1] = tan(m_Camera->GetFov() * 0.5f);
			cameraInfo[4][2] = m_Camera->GetAspect();

			cameraBuffer->Write(cameraInfo);

			primaryBuffer->BeginRenderPass(m_OctreeRayTestPass, SUBPASS_CONTENTS_SECONDARY);

			KRenderCommand command;
			command.vertexData = &m_QuadVertexData;
			command.indexData = &m_QuadIndexData;
			command.pipeline = m_OctreeRayTestPipelines[frameIndex];
			command.pipeline->GetHandle(m_OctreeRayTestPass, command.pipelineHandle);
			command.indexDraw = true;

			commandBuffer->BeginSecondary(m_OctreeRayTestPass);
			commandBuffer->SetViewport(m_OctreeRayTestPass->GetViewPort());
			commandBuffer->Render(command);
			commandBuffer->End();

			primaryBuffer->Execute(commandBuffer);
			primaryBuffer->EndRenderPass();
		}
		return true;
	}
	return false;
}

bool KVoxilzer::UpdateFrame(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex)
{
	bool result = true;
	result &= UpdateLightingResult(primaryBuffer, frameIndex);
	result &= UpdateOctreRayTestResult(primaryBuffer, frameIndex);
	return result;
}

void KVoxilzer::SetupOctreeBuildPipeline()
{
	if (m_VoxelUseOctree)
	{
		m_OctreeTagNodePipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeTagNodePipeline->BindStorageBuffer(OCTREE_BINDING_FRAGMENTLIST, m_FragmentlistBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeTagNodePipeline->BindDynamicUniformBuffer(OCTREE_BINDING_OBJECT);
		m_OctreeTagNodePipeline->Init("voxel/octree_tag_node.comp");

		m_OctreeInitNodePipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeInitNodePipeline->BindStorageBuffer(OCTREE_BINDING_BUILDINFO, m_BuildInfoBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeInitNodePipeline->Init("voxel/octree_init_node.comp");

		m_OctreeAllocNodePipeline->BindStorageBuffer(OCTREE_BINDING_COUNTER, m_CounterBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeAllocNodePipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeAllocNodePipeline->BindStorageBuffer(OCTREE_BINDING_BUILDINFO, m_BuildInfoBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeAllocNodePipeline->Init("voxel/octree_alloc_node.comp");

		m_OctreeModifyArgPipeline->BindStorageBuffer(OCTREE_BINDING_COUNTER, m_CounterBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeModifyArgPipeline->BindStorageBuffer(OCTREE_BINDING_BUILDINFO, m_BuildInfoBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeModifyArgPipeline->BindStorageBuffer(OCTREE_BINDING_INDIRECT, m_BuildIndirectBuffer, COMPUTE_RESOURCE_OUT, true);
		m_OctreeModifyArgPipeline->Init("voxel/octree_modify_arg.comp");

		m_OctreeInitDataPipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeInitDataPipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeInitDataPipeline->BindStorageBuffer(OCTREE_BINDING_BUILDINFO, m_BuildInfoBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeInitDataPipeline->BindStorageBuffer(OCTREE_BINDING_COUNTER, m_CounterBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeInitDataPipeline->Init("voxel/octree_init_data.comp");

		m_OctreeAssignDataPipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeAssignDataPipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeAssignDataPipeline->BindStorageBuffer(OCTREE_BINDING_FRAGMENTLIST, m_FragmentlistBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeAssignDataPipeline->BindDynamicUniformBuffer(OCTREE_BINDING_OBJECT);
		m_OctreeAssignDataPipeline->Init("voxel/octree_assign_data.comp");
	}
}

bool KVoxilzer::Init(IKRenderScene* scene, const KCamera* camera, uint32_t dimension, uint32_t width, uint32_t height)
{
	UnInit();

	m_Scene = scene;
	m_Camera = camera;
	m_Width = width;
	m_Height = height;

	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	renderDevice->CreateVertexBuffer(m_QuadVertexBuffer);
	renderDevice->CreateIndexBuffer(m_QuadIndexBuffer);

	renderDevice->CreateRenderTarget(m_StaticFlag);
	renderDevice->CreateRenderTarget(m_VoxelAlbedo);
	renderDevice->CreateRenderTarget(m_VoxelNormal);
	renderDevice->CreateRenderTarget(m_VoxelEmissive);
	renderDevice->CreateRenderTarget(m_VoxelRadiance);

	renderDevice->CreateRenderTarget(m_LightPassTarget);
	renderDevice->CreateRenderPass(m_LightPassRenderPass);

	renderDevice->CreateRenderTarget(m_OctreeRayTestTarget);
	renderDevice->CreateRenderPass(m_OctreeRayTestPass);

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

	renderDevice->CreateRenderTarget(m_VoxelRenderPassTarget);
	renderDevice->CreateRenderPass(m_VoxelRenderPass);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/light_pass.frag", m_LightPassFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/light_pass_octree.frag", m_LightPassOctreeFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/octree_raytest.frag", m_OctreeRayTestFS, false);

	uint32_t numFrameInFight = renderDevice->GetNumFramesInFlight();

	m_VoxelDrawPipelines.resize(numFrameInFight);
	m_VoxelWireFrameDrawPipelines.resize(numFrameInFight);
	m_VoxelDrawOctreePipelines.resize(numFrameInFight);
	m_VoxelWireFrameDrawOctreePipelines.resize(numFrameInFight);

	m_LightPassPipelines.resize(numFrameInFight);
	m_LightPassOctreePipelines.resize(numFrameInFight);
	m_OctreeRayTestPipelines.resize(numFrameInFight);
	m_DrawCommandBuffers.resize(numFrameInFight);
	m_LightingCommandBuffers.resize(numFrameInFight);
	m_OctreeRayTestCommandBuffers.resize(numFrameInFight);
	m_OctreeCameraBuffers.resize(numFrameInFight);

	for (uint32_t frameIndex = 0; frameIndex < numFrameInFight; ++frameIndex)
	{
		renderDevice->CreatePipeline(m_VoxelDrawPipelines[frameIndex]);
		renderDevice->CreatePipeline(m_VoxelWireFrameDrawPipelines[frameIndex]);
		renderDevice->CreatePipeline(m_VoxelDrawOctreePipelines[frameIndex]);
		renderDevice->CreatePipeline(m_VoxelWireFrameDrawOctreePipelines[frameIndex]);

		renderDevice->CreatePipeline(m_LightPassPipelines[frameIndex]);
		renderDevice->CreatePipeline(m_LightPassOctreePipelines[frameIndex]);
		renderDevice->CreatePipeline(m_OctreeRayTestPipelines[frameIndex]);

		renderDevice->CreateCommandBuffer(m_DrawCommandBuffers[frameIndex]);
		m_DrawCommandBuffers[frameIndex]->Init(m_CommandPool, CBL_SECONDARY);

		renderDevice->CreateCommandBuffer(m_LightingCommandBuffers[frameIndex]);
		m_LightingCommandBuffers[frameIndex]->Init(m_CommandPool, CBL_SECONDARY);

		renderDevice->CreateCommandBuffer(m_OctreeRayTestCommandBuffers[frameIndex]);
		m_OctreeRayTestCommandBuffers[frameIndex]->Init(m_CommandPool, CBL_SECONDARY);
	}

	renderDevice->CreateComputePipeline(m_ClearDynamicPipeline);

	renderDevice->CreateComputePipeline(m_InjectRadiancePipeline);
	renderDevice->CreateComputePipeline(m_InjectPropagationPipeline);

	renderDevice->CreateComputePipeline(m_InjectRadianceOctreePipeline);
	renderDevice->CreateComputePipeline(m_InjectPropagationOctreePipeline);

	renderDevice->CreateComputePipeline(m_MipmapBasePipeline);
	renderDevice->CreateComputePipeline(m_MipmapBaseOctreePipeline);
	renderDevice->CreateComputePipeline(m_MipmapVolumePipeline);

	renderDevice->CreateComputePipeline(m_OctreeMipmapBasePipeline);
	renderDevice->CreateComputePipeline(m_OctreeMipmapVolumePipeline);

	renderDevice->CreateStorageBuffer(m_CounterBuffer);
	renderDevice->CreateStorageBuffer(m_FragmentlistBuffer);
	renderDevice->CreateStorageBuffer(m_CountOnlyBuffer);

	renderDevice->CreateStorageBuffer(m_OctreeBuffer);
	renderDevice->CreateStorageBuffer(m_OctreeDataBuffer);
	renderDevice->CreateStorageBuffer(m_OctreeMipmapDataBuffer);
	renderDevice->CreateStorageBuffer(m_BuildInfoBuffer);
	renderDevice->CreateStorageBuffer(m_BuildIndirectBuffer);

	for (size_t frameIndex = 0; frameIndex < numFrameInFight; ++frameIndex)
	{
		renderDevice->CreateStorageBuffer(m_OctreeCameraBuffers[frameIndex]);
	}

	renderDevice->CreateComputePipeline(m_OctreeTagNodePipeline);
	renderDevice->CreateComputePipeline(m_OctreeInitNodePipeline);
	renderDevice->CreateComputePipeline(m_OctreeAllocNodePipeline);
	renderDevice->CreateComputePipeline(m_OctreeModifyArgPipeline);

	renderDevice->CreateComputePipeline(m_OctreeInitDataPipeline);
	renderDevice->CreateComputePipeline(m_OctreeAssignDataPipeline);

	Resize(width, height);
	SetupQuadDrawData();
	SetupVoxelVolumes(dimension);

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
	m_Camera = nullptr;

	m_LightDebugDrawer.UnInit();
	m_OctreeRayTestDebugDrawer.UnInit();

	KRenderGlobal::ShaderManager.Release(m_QuadVS);
	m_QuadVS = nullptr;
	KRenderGlobal::ShaderManager.Release(m_LightPassFS);
	m_LightPassFS = nullptr;
	KRenderGlobal::ShaderManager.Release(m_LightPassOctreeFS);
	m_LightPassOctreeFS = nullptr;
	KRenderGlobal::ShaderManager.Release(m_OctreeRayTestFS);
	m_OctreeRayTestFS = nullptr;

	SAFE_UNINIT(m_QuadIndexBuffer);
	SAFE_UNINIT(m_QuadVertexBuffer);

	SAFE_UNINIT(m_LightPassTarget);
	SAFE_UNINIT(m_LightPassRenderPass);

	SAFE_UNINIT(m_OctreeRayTestTarget);
	SAFE_UNINIT(m_OctreeRayTestPass);

	SAFE_UNINIT_CONTAINER(m_VoxelDrawPipelines);
	SAFE_UNINIT_CONTAINER(m_VoxelWireFrameDrawPipelines);
	SAFE_UNINIT_CONTAINER(m_VoxelDrawOctreePipelines);
	SAFE_UNINIT_CONTAINER(m_VoxelWireFrameDrawOctreePipelines);

	SAFE_UNINIT_CONTAINER(m_LightPassPipelines);
	SAFE_UNINIT_CONTAINER(m_LightPassOctreePipelines);
	SAFE_UNINIT_CONTAINER(m_OctreeRayTestPipelines);

	SAFE_UNINIT(m_ClearDynamicPipeline);

	SAFE_UNINIT(m_InjectRadiancePipeline);
	SAFE_UNINIT(m_InjectPropagationPipeline);

	SAFE_UNINIT(m_InjectRadianceOctreePipeline);
	SAFE_UNINIT(m_InjectPropagationOctreePipeline);

	SAFE_UNINIT(m_MipmapBasePipeline);
	SAFE_UNINIT(m_MipmapBaseOctreePipeline);
	SAFE_UNINIT(m_MipmapVolumePipeline);

	SAFE_UNINIT(m_OctreeMipmapBasePipeline);
	SAFE_UNINIT(m_OctreeMipmapVolumePipeline);

	SAFE_UNINIT(m_VoxelDrawVS);
	SAFE_UNINIT(m_VoxelDrawOctreeVS);
	SAFE_UNINIT(m_VoxelDrawGS);
	SAFE_UNINIT(m_VoxelDrawFS);

	SAFE_UNINIT(m_VoxelRenderPass);
	SAFE_UNINIT(m_VoxelRenderPassTarget);
	SAFE_UNINIT_CONTAINER(m_DrawCommandBuffers);
	SAFE_UNINIT_CONTAINER(m_LightingCommandBuffers);
	SAFE_UNINIT_CONTAINER(m_OctreeRayTestCommandBuffers);
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

	SAFE_UNINIT(m_CounterBuffer);
	SAFE_UNINIT(m_FragmentlistBuffer);
	SAFE_UNINIT(m_CountOnlyBuffer);

	SAFE_UNINIT(m_OctreeBuffer);
	SAFE_UNINIT(m_OctreeDataBuffer);
	SAFE_UNINIT(m_OctreeMipmapDataBuffer);
	SAFE_UNINIT(m_BuildInfoBuffer);
	SAFE_UNINIT(m_BuildIndirectBuffer);
	SAFE_UNINIT_CONTAINER(m_OctreeCameraBuffers);

	SAFE_UNINIT(m_OctreeTagNodePipeline);
	SAFE_UNINIT(m_OctreeInitNodePipeline);
	SAFE_UNINIT(m_OctreeAllocNodePipeline);
	SAFE_UNINIT(m_OctreeModifyArgPipeline);

	SAFE_UNINIT(m_OctreeInitDataPipeline);
	SAFE_UNINIT(m_OctreeAssignDataPipeline);

	return true;
}