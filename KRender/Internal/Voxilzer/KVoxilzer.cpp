#include "KVoxilzer.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "KBase/Interface/IKLog.h"

const VertexFormat KVoxilzer::ms_VertexFormats[1] = { VF_DEBUG_POINT };

// #define USE_OCTREE_MIPMAP_BUFFER

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
	, m_Enable(true)
	, m_InjectFirstBounce(true)
	, m_VoxelDrawEnable(false)
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

void KVoxilzer::UpdateInternal(KRHICommandListBase& commandList)
{
	UpdateProjectionMatrices(commandList);

	if (m_VoxelUseOctree)
	{
		KRenderGlobal::ImmediateCommandList.BeginRecord();
		VoxelizeStaticSceneCounter(KRenderGlobal::ImmediateCommandList, true);
		KRenderGlobal::ImmediateCommandList.EndRecord();

		KRenderGlobal::ImmediateCommandList.BeginRecord();
		VoxelizeStaticSceneCounter(KRenderGlobal::ImmediateCommandList, false);
		KRenderGlobal::ImmediateCommandList.EndRecord();;

		KRenderGlobal::ImmediateCommandList.BeginRecord();
		// CheckFragmentlistData();
		BuildOctree(KRenderGlobal::ImmediateCommandList);
		KRenderGlobal::ImmediateCommandList.EndRecord();

		uint32_t lastBuildinfo[] = { 0, 0 };
		m_BuildInfoBuffer->Read(lastBuildinfo);
		m_OctreeNonLeafCount = lastBuildinfo[0] + lastBuildinfo[1];
		m_CounterBuffer->Read(&m_OctreeLeafCount);

		KRenderGlobal::ImmediateCommandList.BeginRecord();
		UpdateRadiance(KRenderGlobal::ImmediateCommandList);
		KRenderGlobal::ImmediateCommandList.EndRecord();

		ShrinkOctree();
		CheckOctreeData();
	}
	else
	{
		VoxelizeStaticScene(commandList);
		UpdateRadiance(commandList);
	}
}

void KVoxilzer::OnSceneChanged(EntitySceneOp op, IKEntity* entity)
{
	IKRenderComponent* render = nullptr;
	if (!entity->GetComponent(CT_RENDER, &render) || render->IsUtility())
	{
		return;
	}
	m_VoxelNeedUpdate = true;
}

void KVoxilzer::UpdateVoxel(KRHICommandListBase& commandList)
{
	if (m_Enable)
	{
		if (m_VoxelLastUseOctree != m_VoxelUseOctree)
		{
			FLUSH_INFLIGHT_RENDER_JOB();
			SetupVoxelReleatedData();
		}

		if (m_VoxelDebugUpdate || m_VoxelNeedUpdate || m_VoxelLastUseOctree != m_VoxelUseOctree)
		{
			UpdateInternal(commandList);
			m_VoxelNeedUpdate = false;
			m_VoxelLastUseOctree = m_VoxelUseOctree;
		}
	}
}

void KVoxilzer::Reload()
{
	(*m_VoxelDrawVS)->Reload();
	(*m_VoxelDrawOctreeVS)->Reload();
	(*m_VoxelDrawGS)->Reload();
	(*m_VoxelWireFrameDrawGS)->Reload();
	(*m_VoxelDrawFS)->Reload();
	(*m_QuadVS)->Reload();
	(*m_LightPassFS)->Reload();
	(*m_LightPassOctreeFS)->Reload();
	(*m_OctreeRayTestFS)->Reload();

	if (m_VoxelUseOctree)
	{
		m_OctreeTagNodePipeline->Reload(false);
		m_OctreeInitNodePipeline->Reload(false);
		m_OctreeAllocNodePipeline->Reload(false);
		m_OctreeModifyArgPipeline->Reload(false);

		m_OctreeInitDataPipeline->Reload(false);
		m_OctreeAssignDataPipeline->Reload(false);

		m_InjectRadianceOctreePipeline->Reload(false);
		m_InjectPropagationOctreePipeline->Reload(false);
		m_MipmapBaseOctreePipeline->Reload(false);

		m_VoxelDrawOctreePipeline->Reload(false);
		m_VoxelWireFrameDrawOctreePipeline->Reload(false);
		m_LightPassOctreePipeline->Reload(false);
		m_OctreeRayTestPipeline->Reload(false);
	}
	else
	{
		m_ClearDynamicPipeline->Reload();
		m_InjectRadiancePipeline->Reload();
		m_InjectPropagationPipeline->Reload();

		m_VoxelDrawPipeline->Reload(false);
		m_VoxelWireFrameDrawPipeline->Reload(false);
		m_LightPassPipeline->Reload(false);
	}

	m_MipmapBasePipeline->Reload();
	m_MipmapVolumePipeline->Reload();
}

void KVoxilzer::UpdateProjectionMatrices(KRHICommandListBase& commandList)
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

	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
	
	glm::vec4 minpointScale(min, 1.0f / m_VolumeGridSize);
	glm::vec4 maxpointScale(max, 1.0f / m_VolumeGridSize);

	glm::uvec4 miscs(0);
	// volumeDimension
	miscs[0] = m_VolumeDimension;
	// storeVisibility
	miscs[1] = 1;
	// normalWeightedLambert
	miscs[2] = 1;
	// checkBoundaries
	miscs[3] = 1;

	glm::vec4 miscs2(0);
	// voxelSize
	miscs2[0] = m_VoxelSize;
	// volumeSize
	miscs2[1] = m_VolumeGridSize;
	// exponents
	miscs2[2] = miscs2[3] = 0;

	glm::vec4 miscs3(0);
	// lightBleedingReduction
	miscs3[0] = 0;
	// traceShadowHit
	miscs3[1] = 0.5f;
	// maxTracingDistanceGlobal
	miscs3[2] = 0.10f;

	KConstantDefinition::VOXEL VOXEL;
	for (uint32_t i = 0; i < 3; ++i)
	{
		VOXEL.VIEW_PROJ[i] = m_ViewProjectionMatrix[i];
		VOXEL.VIEW_PROJ_INV[i] = m_ViewProjectionMatrixI[i];
	}
	VOXEL.MINPOINT_SCALE = minpointScale;
	VOXEL.MAXPOINT_SCALE = maxpointScale;
	VOXEL.MISCS = miscs;
	VOXEL.MISCS2 = miscs2;
	VOXEL.MISCS3 = miscs3;

	commandList.UpdateUniformBuffer(voxelBuffer, &VOXEL, 0, sizeof(VOXEL));
}

void KVoxilzer::SetupVoxelBuffer()
{
	uint32_t dimension = m_VolumeDimension;
	m_VoxelAlbedo->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8G8B8A8_UNORM);
	m_VoxelNormal->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8G8B8A8_UNORM);
	m_VoxelRadiance->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8G8B8A8_UNORM);
	m_VoxelEmissive->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8G8B8A8_UNORM);
	m_StaticFlag->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8_UNORM);
}

void KVoxilzer::SetupSparseVoxelBuffer()
{
	uint32_t counter = 0;
	uint32_t countOnly = 1;

	m_CounterBuffer->InitMemory(sizeof(counter), &counter);
	m_CounterBuffer->InitDevice(false, false);

	uint32_t fragmentDummy[] = { 0, 0, 0, 0 };
	m_FragmentlistBuffer->InitMemory(sizeof(fragmentDummy), fragmentDummy);
	m_FragmentlistBuffer->InitDevice(false, false);

	m_CountOnlyBuffer->InitMemory(sizeof(countOnly), &countOnly);
	m_CountOnlyBuffer->InitDevice(false, false);

	uint32_t buildinfo[] = { 0, 8 };
	m_BuildInfoBuffer->InitMemory(sizeof(buildinfo), buildinfo);
	m_BuildInfoBuffer->InitDevice(false, false);

	uint32_t indirectinfo[] = { 1, 1, 1 };
	m_BuildIndirectBuffer->InitMemory(sizeof(indirectinfo), indirectinfo);
	m_BuildIndirectBuffer->InitDevice(true, false);

	// 随意填充空数据即可
	m_OctreeBuffer->InitMemory(sizeof(fragmentDummy), fragmentDummy);
	m_OctreeBuffer->InitDevice(false, false);

	m_OctreeDataBuffer->InitMemory(sizeof(fragmentDummy), fragmentDummy);
	m_OctreeDataBuffer->InitDevice(false, false);

	m_OctreeMipmapDataBuffer->InitMemory(sizeof(fragmentDummy), fragmentDummy);
	m_OctreeMipmapDataBuffer->InitDevice(false, false);

	glm::vec4 cameraDummy[5] = { glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, -1, 0), glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 0, 0) };

	m_OctreeCameraBuffer->InitMemory(sizeof(cameraDummy), cameraDummy);
	m_OctreeCameraBuffer->InitDevice(false, false);

	uint32_t dimension = m_VolumeDimension;
	m_StaticFlag->InitFromStorage3D(dimension, dimension, dimension, 1, EF_R8_UNORM);
}

void KVoxilzer::SetupVoxelReleatedData()
{
	uint32_t dimension = m_VolumeDimension;
	uint32_t baseMipmapDimension = (dimension + 1) / 2;

	m_VoxelRenderPassTarget->InitFromColor(dimension, dimension, 1, 1, EF_R8_UNORM);
	m_VoxelRenderPass->UnInit();
	m_VoxelRenderPass->SetColorAttachment(0, m_VoxelRenderPassTarget->GetFrameBuffer());
	m_VoxelRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_VoxelRenderPass->Init();

	m_LightPassTarget->InitFromColor(m_Width, m_Height, 1, 1, EF_R8G8B8A8_UNORM);
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
		m_VoxelDrawPipeline->UnInit();
		m_VoxelWireFrameDrawPipeline->UnInit();
		m_LightPassPipeline->UnInit();
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
		m_OctreeCameraBuffer->UnInit();
		m_StaticFlag->UnInit();
		SetupVoxelBuffer();
		m_VoxelDrawOctreePipeline->UnInit();
		m_VoxelWireFrameDrawOctreePipeline->UnInit();
		m_LightPassOctreePipeline->UnInit();
	}

	for (uint32_t i = 0; i < 6; ++i)
	{
		m_VoxelTexMipmap[i]->InitFromStorage3D(baseMipmapDimension, baseMipmapDimension, baseMipmapDimension, m_NumMipmap, EF_R8G8B8A8_UNORM);
	}

	SetupVoxelDrawPipeline();
	SetupClearDynamicPipeline();
	SetupRadiancePipeline();
	SetupMipmapPipeline();
	SetupOctreeMipmapPipeline();
	SetupLightPassPipeline();

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

	KRenderGlobal::ImmediateCommandList.BeginRecord();
	UpdateProjectionMatrices(KRenderGlobal::ImmediateCommandList);
	KRenderGlobal::ImmediateCommandList.EndRecord();

	SetupVoxelReleatedData();
}

void KVoxilzer::SetupVoxelDrawPipeline()
{
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/svo/draw/draw_voxels.vert", m_VoxelDrawVS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/svo/draw/draw_voxels_octree.vert", m_VoxelDrawOctreeVS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/svo/draw/draw_voxels.geom", m_VoxelDrawGS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/svo/draw/draw_voxels_wireframe.geom", m_VoxelWireFrameDrawGS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/svo/draw/draw_voxels.frag", m_VoxelDrawFS, false));

	enum
	{
		DEFAULT,
		WIREFRAME,
		COUNT
	};

	for (size_t idx = 0; idx < COUNT; ++idx)
	{
		IKPipelinePtr* pPipeline = nullptr;

		if (m_VoxelUseOctree)
		{
			pPipeline = idx == DEFAULT ? &m_VoxelDrawOctreePipeline : &m_VoxelWireFrameDrawOctreePipeline;
		}
		else
		{
			pPipeline = idx == DEFAULT ? &m_VoxelDrawPipeline : &m_VoxelWireFrameDrawPipeline;
		}

		{
			IKPipelinePtr pipeline = *pPipeline;

			pipeline->SetShader(ST_VERTEX, m_VoxelUseOctree ? *m_VoxelDrawOctreeVS : *m_VoxelDrawVS);

			pipeline->SetPrimitiveTopology(PT_POINT_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_NONE);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetShader(ST_GEOMETRY, idx == 0 ? *m_VoxelDrawGS : *m_VoxelWireFrameDrawGS);
			pipeline->SetShader(ST_FRAGMENT, *m_VoxelDrawFS);

			IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
			pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelBuffer);

			IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
			pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, cameraBuffer);

			if (m_VoxelUseOctree)
			{
				pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE, ST_VERTEX, m_OctreeBuffer);
				pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE_DATA, ST_VERTEX, m_OctreeDataBuffer);
				pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE_MIPMAP_DATA, ST_VERTEX, m_OctreeMipmapDataBuffer);
				std::vector<IKFrameBufferPtr> targets;
				std::vector<IKSamplerPtr> samplers;
				targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
				samplers.resize(ARRAY_SIZE(m_VoxelTexMipmap));
				for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
				{
					targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
					samplers[i] = m_MipmapSampler;
				}
				pipeline->SetSamplers(VOXEL_BINDING_TEXMIPMAP_OUT, targets, samplers, true);
				pipeline->SetStorageImage(VOXEL_BINDING_TEXMIPMAP_IN, m_VoxelTexMipmap[0]->GetFrameBuffer(), EF_UNKNOWN);
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
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);

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

		m_ClearDynamicPipeline->Init("voxel/svo/lighting/clear_dynamic.comp", KShaderCompileEnvironment());
	}
}

void KVoxilzer::SetupRadiancePipeline()
{
	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);

	// Inject Radiance
	IKComputePipelinePtr& injectRadiancePipeline = m_VoxelUseOctree ? m_InjectRadianceOctreePipeline : m_InjectRadiancePipeline;

	injectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_GLOBAL, globalBuffer);
	injectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);

	if (m_VoxelUseOctree)
	{
		injectRadiancePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		injectRadiancePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		injectRadiancePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_MIPMAP_DATA, m_OctreeMipmapDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		injectRadiancePipeline->Init("voxel/svo/lighting/inject_radiance_octree.comp", KShaderCompileEnvironment());
	}
	else
	{
		injectRadiancePipeline->BindSampler(VOXEL_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), m_LinearSampler, true);
		injectRadiancePipeline->BindStorageImage(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, true);
		injectRadiancePipeline->BindStorageImage(VOXEL_BINDING_EMISSION_MAP, m_VoxelEmissive->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);
		injectRadiancePipeline->BindStorageImage(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
		injectRadiancePipeline->Init("voxel/svo/lighting/inject_radiance.comp", KShaderCompileEnvironment());
	}

	// Inject Propagation
	IKComputePipelinePtr& injectPropagationPipeline = m_VoxelUseOctree ? m_InjectPropagationOctreePipeline : m_InjectPropagationPipeline;

	injectPropagationPipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);

	if (m_VoxelUseOctree)
	{
		injectPropagationPipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		injectPropagationPipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		injectPropagationPipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_MIPMAP_DATA, m_OctreeMipmapDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
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
		injectPropagationPipeline->Init("voxel/svo/lighting/inject_propagation_octree.comp", KShaderCompileEnvironment());
	}
	else
	{
		injectPropagationPipeline->Init("voxel/svo/lighting/inject_propagation.comp", KShaderCompileEnvironment());
	}
}

void KVoxilzer::SetupMipmapPipeline()
{
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);

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
		mipmapBasePipeline->Init("voxel/svo/lighting/aniso_mipmapbase_octree.comp", KShaderCompileEnvironment());
	}
	else
	{
		mipmapBasePipeline->BindSampler(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), m_LinearSampler, false);
		mipmapBasePipeline->Init("voxel/svo/lighting/aniso_mipmapbase.comp", KShaderCompileEnvironment());
	}

	m_MipmapVolumePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
	m_MipmapVolumePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_MipmapVolumePipeline->BindSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers, false);
	m_MipmapVolumePipeline->BindStorageImages(VOXEL_BINDING_TEXMIPMAP_OUT, targets, EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);

	m_MipmapVolumePipeline->Init("voxel/svo/lighting/aniso_mipmapvolume.comp", KShaderCompileEnvironment());
}

void KVoxilzer::SetupOctreeMipmapPipeline()
{
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);

	m_OctreeMipmapBasePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
	m_OctreeMipmapBasePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_OctreeMipmapBasePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapBasePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapBasePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_MIPMAP_DATA, m_OctreeMipmapDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapBasePipeline->Init("voxel/svo/lighting/aniso_octree_mipmapbase.comp", KShaderCompileEnvironment());

	m_OctreeMipmapVolumePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, voxelBuffer);
	m_OctreeMipmapVolumePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_OctreeMipmapVolumePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapVolumePipeline->BindStorageBuffer(VOXEL_BINDING_OCTREE_MIPMAP_DATA, m_OctreeMipmapDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
	m_OctreeMipmapVolumePipeline->Init("voxel/svo/lighting/aniso_octree_mipmapvolume.comp", KShaderCompileEnvironment());
}

void KVoxilzer::Resize(uint32_t width, uint32_t height)
{
	m_LightPassTarget->UnInit();
	m_LightPassTarget->InitFromColor(width, height, 1, 1, EF_R8G8B8A8_UNORM);

	m_LightPassRenderPass->UnInit();
	m_LightPassRenderPass->SetColorAttachment(0, m_LightPassTarget->GetFrameBuffer());
	m_LightPassRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_LightPassRenderPass->Init();

	m_OctreeRayTestTarget->UnInit();
	m_OctreeRayTestPass->UnInit();

	m_OctreeRayTestTarget->InitFromColor(width, height, 1, 1, EF_R8G8B8A8_UNORM);
	m_OctreeRayTestPass->SetColorAttachment(0, m_OctreeRayTestTarget->GetFrameBuffer());
	m_OctreeRayTestPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_OctreeRayTestPass->Init();

	// 清空RenderTarget同时Translate Layout到Shader可读
	KRenderGlobal::ImmediateCommandList.BeginRecord();

	KRenderGlobal::ImmediateCommandList.BeginRenderPass(m_LightPassRenderPass, SUBPASS_CONTENTS_INLINE);
	KRenderGlobal::ImmediateCommandList.SetViewport(m_LightPassRenderPass->GetViewPort());
	KRenderGlobal::ImmediateCommandList.EndRenderPass();

	KRenderGlobal::ImmediateCommandList.BeginRenderPass(m_OctreeRayTestPass, SUBPASS_CONTENTS_INLINE);
	KRenderGlobal::ImmediateCommandList.SetViewport(m_OctreeRayTestPass->GetViewPort());
	KRenderGlobal::ImmediateCommandList.EndRenderPass();

	KRenderGlobal::ImmediateCommandList.EndRecord();
}

void KVoxilzer::SetupLightPassPipeline()
{
	m_LightDebugDrawer.Init(m_LightPassTarget->GetFrameBuffer(), 0, 0, 1, 1);

	std::vector<IKFrameBufferPtr> targets;
	std::vector<IKSamplerPtr> samplers;
	targets.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	samplers.resize(ARRAY_SIZE(m_VoxelTexMipmap));
	for (size_t i = 0; i < ARRAY_SIZE(m_VoxelTexMipmap); ++i)
	{
		targets[i] = m_VoxelTexMipmap[i]->GetFrameBuffer();
		samplers[i] = m_MipmapSampler;
	}

	IKPipelinePtr& pipeline = m_VoxelUseOctree ? m_LightPassOctreePipeline : m_LightPassPipeline;

	pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	pipeline->SetShader(ST_VERTEX, *m_QuadVS);

	if (m_VoxelUseOctree)
	{
		pipeline->SetShader(ST_FRAGMENT, *m_LightPassOctreeFS);
	}
	else
	{
		pipeline->SetShader(ST_FRAGMENT, *m_LightPassFS);
	}

	pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	pipeline->SetBlendEnable(false);
	pipeline->SetCullMode(CM_NONE);
	pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	pipeline->SetPolygonMode(PM_FILL);
	pipeline->SetColorWrite(true, true, true, true);
	pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
	pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelBuffer);

	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
	pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, cameraBuffer);

	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
	pipeline->SetConstantBuffer(CBT_GLOBAL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, globalBuffer);

	pipeline->SetSampler(VOXEL_BINDING_GBUFFER_RT0,
		KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);
	pipeline->SetSampler(VOXEL_BINDING_GBUFFER_RT1,
		KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET1)->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);
	pipeline->SetSampler(VOXEL_BINDING_GBUFFER_RT2,
		KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET2)->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);
	pipeline->SetSampler(VOXEL_BINDING_GBUFFER_RT3,
		KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET3)->GetFrameBuffer(),
		KRenderGlobal::GBuffer.GetSampler(),
		true);

	if (m_VoxelUseOctree)
	{
		pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE, ST_FRAGMENT, m_OctreeBuffer);
		pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE_DATA, ST_FRAGMENT, m_OctreeDataBuffer);
		pipeline->SetStorageBuffer(VOXEL_BINDING_OCTREE_MIPMAP_DATA, ST_FRAGMENT, m_OctreeMipmapDataBuffer);
	}
	else
	{
		pipeline->SetSampler(VOXEL_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), m_LinearSampler, false);
		pipeline->SetSampler(VOXEL_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), m_LinearSampler, false);
	}

	pipeline->SetSamplers(VOXEL_BINDING_TEXMIPMAP_IN, targets, samplers, false);

	pipeline->Init();
}

void KVoxilzer::SetupRayTestPipeline(uint32_t width, uint32_t height)
{
	m_OctreeRayTestDebugDrawer.Init(m_OctreeRayTestTarget->GetFrameBuffer(), 0, 0, 1, 1);

	if (m_VoxelUseOctree)
	{
		IKPipelinePtr& pipeline = m_OctreeRayTestPipeline;

		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_OctreeRayTestFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetStorageBuffer(OCTREE_BINDING_OCTREE, ST_FRAGMENT, m_OctreeBuffer);
		pipeline->SetStorageBuffer(OCTREE_BINDING_OCTREE_DATA, ST_FRAGMENT, m_OctreeDataBuffer);
		pipeline->SetStorageBuffer(OCTREE_BINDING_CAMERA, ST_FRAGMENT, m_OctreeCameraBuffer);

		pipeline->Init();
	}
	else
	{
		m_OctreeRayTestPipeline->UnInit();
	}
}

void KVoxilzer::ClearDynamicScene(KRHICommandListBase& commandList)
{
	uint32_t group = (m_VolumeDimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;
	commandList.Execute(m_ClearDynamicPipeline, group, group, group, 0);
}

void KVoxilzer::VoxelizeStaticScene(KRHICommandListBase& commandList)
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	// TODO
	std::vector<IKEntity*> cullRes;
	m_Scene->GetVisibleEntities(sceneBox, cullRes);

	if (cullRes.size() == 0) return;

	commandList.BeginDebugMarker("VoxelizeStaticScene", glm::vec4(1));
	commandList.BeginRenderPass(m_VoxelRenderPass, SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_VoxelRenderPass->GetViewPort());

	std::vector<KRenderCommand> commands;
	for (IKEntity* entity : cullRes)
	{
		KRenderComponent* render = nullptr;
		KTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_RENDER, &render) && entity->GetComponent(CT_TRANSFORM, &transform))
		{
			const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();
			for (KMaterialSubMeshPtr materialSubMesh : materialSubMeshes)
			{
				KRenderCommand command;
				if (materialSubMesh->GetRenderCommand(RENDER_STAGE_VOXEL, command))
				{
					const glm::mat4& finalTran = transform->GetFinal_RenderThread();

					struct ObjectData
					{
						glm::mat4 model;
					}objectData;
					objectData.model = finalTran;


					KDynamicConstantBufferUsage objectUsage;
					objectUsage.binding = SHADER_BINDING_OBJECT;
					objectUsage.range = sizeof(objectData);

					KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

					command.dynamicConstantUsages.push_back(objectUsage);

					command.pipeline->GetHandle(m_VoxelRenderPass, command.pipelineHandle);

					commands.push_back(command);
				}
			}
		}
	}

	for (KRenderCommand& command : commands)
	{
		commandList.Render(command);
	}

	commandList.EndRenderPass();
	commandList.EndDebugMarker();
}

void KVoxilzer::VoxelizeStaticSceneCounter(KRHICommandListBase& commandList, bool bCountOnly)
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	std::vector<IKEntity*> cullRes;
	m_Scene->GetVisibleEntities(sceneBox, cullRes);

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
			m_FragmentlistBuffer->InitDevice(false, false);
		}

		counter = 0;
	}

	m_CounterBuffer->Write(&counter);
	m_CountOnlyBuffer->Write(&countOnly);

	if (cullRes.size() == 0) return;

	commandList.BeginRenderPass(m_VoxelRenderPass, SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(m_VoxelRenderPass->GetViewPort());

	std::vector<KRenderCommand> commands;

	for (IKEntity* entity : cullRes)
	{
		KRenderComponent* render = nullptr;
		KTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_RENDER, &render) && entity->GetComponent(CT_TRANSFORM, &transform))
		{
			const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();
			for (KMaterialSubMeshPtr materialSubMesh : materialSubMeshes)
			{
				KRenderCommand command;
				if (materialSubMesh->GetRenderCommand(RENDER_STAGE_SPARSE_VOXEL, command))
				{
					const glm::mat4& finalTran = transform->GetFinal_RenderThread();
					const glm::mat4& prevFinalTran = transform->GetPrevFinal_RenderThread();

					KConstantDefinition::OBJECT objectData;
					objectData.MODEL = finalTran;
					objectData.PRVE_MODEL = prevFinalTran;

					KDynamicConstantBufferUsage objectUsage;
					objectUsage.binding = SHADER_BINDING_OBJECT;
					objectUsage.range = sizeof(objectData);

					KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

					command.dynamicConstantUsages.push_back(objectUsage);

					command.pipeline->GetHandle(m_VoxelRenderPass, command.pipelineHandle);

					commands.push_back(command);
				}
			}
		}
	}

	for (KRenderCommand& command : commands)
	{
		commandList.Render(command);
	}

	commandList.EndRenderPass();
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
	
#ifdef USE_OCTREE_MIPMAP_BUFFER
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
		uint32_t* encodeds = mipmapDatas[i].data;
		for (size_t j = 0; j < 6; ++j)
		{
			uint32_t encoded = encodeds[j];
			glm::uvec4 unpacked = glm::uvec4(encoded & 0xff, (encoded >> 8) & 0xff, (encoded >> 16) & 0xff, (encoded >> 24) & 0xff);
			glm::vec4 data = glm::vec4(unpacked) / 255.0f;
			float luminance = glm::dot(data, glm::vec4(0.2126f, 0.7152f, 0.0722f, 0.0f));
			if (luminance == 0.0f)
			{
				// KG_LOG(LM_DEFAULT, "Miapmap voxel radiance miss");
			}
		}
	}
#endif
}

inline static constexpr uint32_t group_x_64(uint32_t x) { return (x >> 6u) + ((x & 0x3fu) ? 1u : 0u); }

void KVoxilzer::BuildOctree(KRHICommandListBase& commandList)
{
	uint32_t fragmentCount = 0;
	m_CounterBuffer->Read(&fragmentCount);

	// In case 0 size
	fragmentCount = std::max(fragmentCount, 1u);

	// Estimate octree buffer size
	uint32_t octreeNodeNum = std::max((uint32_t)OCTREE_NODE_NUM_MIN, fragmentCount << 2u);
	octreeNodeNum = std::min(octreeNodeNum, (uint32_t)OCTREE_NODE_NUM_MAX);

	uint32_t preOctreeNodeNum = (uint32_t)m_OctreeBuffer->GetBufferSize() / OCTREE_NODE_SIZE;
	if (octreeNodeNum > preOctreeNodeNum)
	{
		m_OctreeBuffer->UnInit();
		m_OctreeBuffer->InitMemory(octreeNodeNum * OCTREE_NODE_SIZE, nullptr);
		m_OctreeBuffer->InitDevice(false, false);
		m_OctreeBuffer->SetDebugName("SVO_OctreeBuffer");

#ifdef USE_OCTREE_MIPMAP_BUFFER
		m_OctreeMipmapDataBuffer->UnInit();
		m_OctreeMipmapDataBuffer->InitMemory(octreeNodeNum * OCTREE_MIPMAP_DATA_SIZE, nullptr);
		m_OctreeMipmapDataBuffer->InitDevice(false);
		m_OctreeMipmapDataBuffer->SetDebugName("SVO_OctreeMipmapDataBuffer");
#endif
	}

	m_OctreeDataBuffer->UnInit();
	m_OctreeDataBuffer->InitMemory(fragmentCount * OCTREE_DATA_SIZE, nullptr);
	m_OctreeDataBuffer->InitDevice(false, false);
	m_OctreeDataBuffer->SetDebugName("SVO_OctreeDataBuffer");

	uint32_t buildinfo[] = { 0, 8 };
	m_BuildInfoBuffer->InitMemory(sizeof(buildinfo), buildinfo);
	m_BuildInfoBuffer->InitDevice(false, false);
	m_BuildInfoBuffer->SetDebugName("SVO_BuildInfoBuffer");

	uint32_t indirectinfo[] = { 1, 1, 1 };
	m_BuildIndirectBuffer->InitMemory(sizeof(indirectinfo), indirectinfo);
	m_BuildIndirectBuffer->InitDevice(true, false);
	m_BuildIndirectBuffer->SetDebugName("SVO_BuildIndirectBuffer");

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
		commandList.ExecuteIndirect(m_OctreeInitNodePipeline, m_BuildIndirectBuffer, nullptr);
		commandList.Execute(m_OctreeTagNodePipeline, fragmentGroupX, 1, 1, &usage);
		if (i != m_OctreeLevel)
		{
			commandList.ExecuteIndirect(m_OctreeAllocNodePipeline, m_BuildIndirectBuffer,nullptr);
			commandList.Execute(m_OctreeModifyArgPipeline, 1, 1, 1, nullptr);
		}
		else
		{
			uint32_t counter = 0;
			m_CounterBuffer->Write(&counter);
			commandList.ExecuteIndirect(m_OctreeInitDataPipeline, m_BuildIndirectBuffer, nullptr);
			commandList.Execute(m_OctreeAssignDataPipeline, fragmentGroupX, 1, 1, &usage);
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
	m_OctreeBuffer->InitDevice(false, false);
	m_OctreeBuffer->SetDebugName("SVO_OctreeBuffer");

#ifdef USE_OCTREE_MIPMAP_BUFFER
	buffers.resize(m_OctreeMipmapDataBuffer->GetBufferSize());
	m_OctreeMipmapDataBuffer->Read(buffers.data());
	m_OctreeMipmapDataBuffer->UnInit();
	m_OctreeMipmapDataBuffer->InitMemory(m_OctreeNonLeafCount * OCTREE_MIPMAP_DATA_SIZE, buffers.data());
	m_OctreeMipmapDataBuffer->InitDevice(false);
	m_OctreeMipmapDataBuffer->SetDebugName("SVO_OctreeMipmapDataBuffer");
#endif

	buffers.resize(m_OctreeDataBuffer->GetBufferSize());
	m_OctreeDataBuffer->Read(buffers.data());
	m_OctreeDataBuffer->UnInit();
	m_OctreeDataBuffer->InitMemory(m_OctreeLeafCount * OCTREE_DATA_SIZE, buffers.data());
	m_OctreeDataBuffer->InitDevice(false, false);
	m_OctreeDataBuffer->SetDebugName("SVO_OctreeDataBuffer");
}

void KVoxilzer::UpdateRadiance(KRHICommandListBase& commandList)
{
	InjectRadiance(commandList);
	GenerateMipmap(commandList);

	if (m_InjectFirstBounce)
	{
		uint32_t group = (m_VolumeDimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;
		IKComputePipelinePtr& propagationPipeline = m_VoxelUseOctree ? m_InjectPropagationOctreePipeline : m_InjectPropagationPipeline;
		commandList.Execute(propagationPipeline, group, group, group, 0);
		GenerateMipmap(commandList);
	}
}

void KVoxilzer::InjectRadiance(KRHICommandListBase& commandList)
{
	uint32_t group = (m_VolumeDimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;
	IKComputePipelinePtr& injectPipeline = m_VoxelUseOctree ? m_InjectRadianceOctreePipeline : m_InjectRadiancePipeline;
	commandList.Execute(injectPipeline, group, group, group, 0);
}

void KVoxilzer::GenerateMipmap(KRHICommandListBase& commandList)
{
	if (m_VoxelUseOctree)
	{
#ifdef USE_OCTREE_MIPMAP_BUFFER
		GenerateOctreeMipmapBase(commandList);
		GenerateOctreeMipmapVolume(commandList);
#else
		GenerateMipmapBase(commandList);
		GenerateMipmapVolume(commandList);
#endif
	}
	else
	{
		GenerateMipmapBase(commandList);
		GenerateMipmapVolume(commandList);
	}
}

void KVoxilzer::GenerateMipmapBase(KRHICommandListBase& commandList)
{
	uint32_t dimension = m_VolumeDimension / 2;
	uint32_t group = (dimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;

	glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, 0);

	KDynamicConstantBufferUsage usage;
	usage.binding = SHADER_BINDING_OBJECT;
	usage.range = sizeof(constant);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

	IKComputePipelinePtr& mipmapBasePipeline = m_VoxelUseOctree ? m_MipmapBaseOctreePipeline : m_MipmapBasePipeline;
	commandList.Execute(mipmapBasePipeline, group, group, group, &usage);
}

void KVoxilzer::GenerateMipmapVolume(KRHICommandListBase& commandList)
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
			commandList.Execute(m_MipmapVolumePipeline, group, group, group, &usage);
		}

		dimension /= 2;
		++mipmap;
	}

	ASSERT_RESULT(mipmap == m_NumMipmap);
}

void KVoxilzer::GenerateOctreeMipmapBase(KRHICommandListBase& commandList)
{
	uint32_t dimension = m_VolumeDimension / 2;
	uint32_t group = (dimension + (VOXEL_GROUP_SIZE - 1)) / VOXEL_GROUP_SIZE;

	glm::uvec4 constant = glm::uvec4(dimension, dimension, dimension, 0);

	KDynamicConstantBufferUsage usage;
	usage.binding = SHADER_BINDING_OBJECT;
	usage.range = sizeof(constant);
	KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, usage);

	commandList.Execute(m_OctreeMipmapBasePipeline, group, group, group, &usage);
}

void KVoxilzer::GenerateOctreeMipmapVolume(KRHICommandListBase& commandList)
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

		commandList.Execute(m_OctreeMipmapVolumePipeline, group, group, group, &usage);

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

bool KVoxilzer::DebugRender(IKRenderPassPtr renderPass, KRHICommandListBase& commandList)
{
	return m_LightDebugDrawer.Render(renderPass, commandList);
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

bool KVoxilzer::OctreeRayTestRender(IKRenderPassPtr renderPass, KRHICommandListBase& commandList)
{
	return m_OctreeRayTestDebugDrawer.Render(renderPass, commandList);
}

bool KVoxilzer::RenderVoxel(IKRenderPassPtr renderPass, KRHICommandListBase& commandList)
{
	if (!m_VoxelDrawEnable || !m_Enable)
		return true;

	IKPipelinePtr* pipeline = nullptr;

	if (m_VoxelUseOctree)
	{
		pipeline = m_VoxelDrawWireFrame ? &m_VoxelWireFrameDrawOctreePipeline : &m_VoxelDrawOctreePipeline;
	}
	else
	{
		pipeline = m_VoxelDrawWireFrame ? &m_VoxelWireFrameDrawPipeline : &m_VoxelDrawPipeline;
	}

	KRenderCommand command;
	command.vertexData = &m_VoxelDrawVertexData;
	command.indexData = nullptr;
	command.pipeline = *pipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = false;

	commandList.Render(command);

	return true;
}

bool KVoxilzer::UpdateLightingResult(KRHICommandListBase& commandList)
{
	if (m_Enable)
	{
		IKPipelinePtr& lightPassPipeline = m_VoxelUseOctree ? m_LightPassOctreePipeline : m_LightPassPipeline;

		commandList.BeginDebugMarker("VoxelLightPass", glm::vec4(1));
		commandList.BeginRenderPass(m_LightPassRenderPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(m_LightPassRenderPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.pipeline = lightPassPipeline;
		command.pipeline->GetHandle(m_LightPassRenderPass, command.pipelineHandle);
		command.indexDraw = true;

		commandList.Render(command);

		commandList.EndRenderPass();
		commandList.EndDebugMarker();

		commandList.Transition(m_LightPassTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	return true;
}

bool KVoxilzer::UpdateOctreRayTestResult(KRHICommandListBase& commandList)
{
	if (m_VoxelUseOctree && m_OctreeRayTestDebugDrawer.GetEnable())
	{
		IKStorageBufferPtr cameraBuffer = m_OctreeCameraBuffer;

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

		commandList.BeginDebugMarker("VoxelOctreeRayTest", glm::vec4(1));
		commandList.BeginRenderPass(m_OctreeRayTestPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(m_OctreeRayTestPass->GetViewPort());

		KRenderCommand command;
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.pipeline = m_OctreeRayTestPipeline;
		command.pipeline->GetHandle(m_OctreeRayTestPass, command.pipelineHandle);
		command.indexDraw = true;

		commandList.Render(command);

		commandList.EndRenderPass();
		commandList.EndDebugMarker();

		commandList.Transition(m_OctreeRayTestTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}
	return true;
}

bool KVoxilzer::UpdateFrame(KRHICommandListBase& commandList)
{
	bool result = true;
	UpdateProjectionMatrices(commandList);
	if (m_Enable)
	{
		result &= UpdateLightingResult(commandList);
		result &= UpdateOctreRayTestResult(commandList);
	}
	else
	{
		commandList.BeginDebugMarker("VoxelLightPass", glm::vec4(1));
		commandList.BeginRenderPass(m_LightPassRenderPass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(m_LightPassRenderPass->GetViewPort());
		commandList.EndRenderPass();
		commandList.EndDebugMarker();
		commandList.Transition(m_LightPassTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}
	return result;
}

void KVoxilzer::SetupOctreeBuildPipeline()
{
	if (m_VoxelUseOctree)
	{
		m_OctreeTagNodePipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeTagNodePipeline->BindStorageBuffer(OCTREE_BINDING_FRAGMENTLIST, m_FragmentlistBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeTagNodePipeline->BindDynamicUniformBuffer(OCTREE_BINDING_OBJECT);
		m_OctreeTagNodePipeline->Init("voxel/svo/octree/octree_tag_node.comp", KShaderCompileEnvironment());

		m_OctreeInitNodePipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeInitNodePipeline->BindStorageBuffer(OCTREE_BINDING_BUILDINFO, m_BuildInfoBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeInitNodePipeline->Init("voxel/svo/octree/octree_init_node.comp", KShaderCompileEnvironment());

		m_OctreeAllocNodePipeline->BindStorageBuffer(OCTREE_BINDING_COUNTER, m_CounterBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeAllocNodePipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeAllocNodePipeline->BindStorageBuffer(OCTREE_BINDING_BUILDINFO, m_BuildInfoBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeAllocNodePipeline->Init("voxel/svo/octree/octree_alloc_node.comp", KShaderCompileEnvironment());

		m_OctreeModifyArgPipeline->BindStorageBuffer(OCTREE_BINDING_COUNTER, m_CounterBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeModifyArgPipeline->BindStorageBuffer(OCTREE_BINDING_BUILDINFO, m_BuildInfoBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeModifyArgPipeline->BindStorageBuffer(OCTREE_BINDING_INDIRECT, m_BuildIndirectBuffer, COMPUTE_RESOURCE_OUT, true);
		m_OctreeModifyArgPipeline->Init("voxel/svo/octree/octree_modify_arg.comp", KShaderCompileEnvironment());

		m_OctreeInitDataPipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeInitDataPipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeInitDataPipeline->BindStorageBuffer(OCTREE_BINDING_BUILDINFO, m_BuildInfoBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeInitDataPipeline->BindStorageBuffer(OCTREE_BINDING_COUNTER, m_CounterBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeInitDataPipeline->Init("voxel/svo/octree/octree_init_data.comp", KShaderCompileEnvironment());

		m_OctreeAssignDataPipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE, m_OctreeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeAssignDataPipeline->BindStorageBuffer(OCTREE_BINDING_OCTREE_DATA, m_OctreeDataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
		m_OctreeAssignDataPipeline->BindStorageBuffer(OCTREE_BINDING_FRAGMENTLIST, m_FragmentlistBuffer, COMPUTE_RESOURCE_IN, true);
		m_OctreeAssignDataPipeline->BindDynamicUniformBuffer(OCTREE_BINDING_OBJECT);
		m_OctreeAssignDataPipeline->Init("voxel/svo/octree/octree_assign_data.comp", KShaderCompileEnvironment());
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

	renderDevice->CreateRenderTarget(m_VoxelRenderPassTarget);
	renderDevice->CreateRenderPass(m_VoxelRenderPass);

	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/svo/lighting/light_pass.frag", m_LightPassFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/svo/lighting/light_pass_octree.frag", m_LightPassOctreeFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/svo/octree/octree_raytest.frag", m_OctreeRayTestFS, false);

	renderDevice->CreatePipeline(m_VoxelDrawPipeline);
	renderDevice->CreatePipeline(m_VoxelWireFrameDrawPipeline);
	renderDevice->CreatePipeline(m_VoxelDrawOctreePipeline);
	renderDevice->CreatePipeline(m_VoxelWireFrameDrawOctreePipeline);

	renderDevice->CreatePipeline(m_LightPassPipeline);
	renderDevice->CreatePipeline(m_LightPassOctreePipeline);
	renderDevice->CreatePipeline(m_OctreeRayTestPipeline);

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
	renderDevice->CreateStorageBuffer(m_OctreeCameraBuffer);

	renderDevice->CreateComputePipeline(m_OctreeTagNodePipeline);
	renderDevice->CreateComputePipeline(m_OctreeInitNodePipeline);
	renderDevice->CreateComputePipeline(m_OctreeAllocNodePipeline);
	renderDevice->CreateComputePipeline(m_OctreeModifyArgPipeline);

	renderDevice->CreateComputePipeline(m_OctreeInitDataPipeline);
	renderDevice->CreateComputePipeline(m_OctreeAssignDataPipeline);

	Resize(width, height);
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

	m_QuadVS.Release();
	m_LightPassFS.Release();
	m_LightPassOctreeFS.Release();
	m_OctreeRayTestFS.Release();

	SAFE_UNINIT(m_LightPassTarget);
	SAFE_UNINIT(m_LightPassRenderPass);

	SAFE_UNINIT(m_OctreeRayTestTarget);
	SAFE_UNINIT(m_OctreeRayTestPass);

	SAFE_UNINIT(m_VoxelDrawPipeline);
	SAFE_UNINIT(m_VoxelWireFrameDrawPipeline);
	SAFE_UNINIT(m_VoxelDrawOctreePipeline);
	SAFE_UNINIT(m_VoxelWireFrameDrawOctreePipeline);

	SAFE_UNINIT(m_LightPassPipeline);
	SAFE_UNINIT(m_LightPassOctreePipeline);
	SAFE_UNINIT(m_OctreeRayTestPipeline);

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

	m_VoxelDrawVS.Release();
	m_VoxelDrawOctreeVS.Release();
	m_VoxelDrawGS.Release();
	m_VoxelWireFrameDrawGS.Release();
	m_VoxelDrawFS.Release();

	SAFE_UNINIT(m_VoxelRenderPass);
	SAFE_UNINIT(m_VoxelRenderPassTarget);

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
	SAFE_UNINIT(m_OctreeCameraBuffer);

	SAFE_UNINIT(m_OctreeTagNodePipeline);
	SAFE_UNINIT(m_OctreeInitNodePipeline);
	SAFE_UNINIT(m_OctreeAllocNodePipeline);
	SAFE_UNINIT(m_OctreeModifyArgPipeline);

	SAFE_UNINIT(m_OctreeInitDataPipeline);
	SAFE_UNINIT(m_OctreeAssignDataPipeline);

	return true;
}