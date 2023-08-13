#include "KVirtualGeometryScene.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "KBase/Publish/KMath.h"

KVirtualGeometryScene::KVirtualGeometryScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
{
	m_OnSceneChangedFunc = std::bind(&KVirtualGeometryScene::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
	m_OnRenderComponentChangedFunc = std::bind(&KVirtualGeometryScene::OnRenderComponentChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KVirtualGeometryScene::~KVirtualGeometryScene()
{
	assert(m_Instances.empty());
}

void KVirtualGeometryScene::OnSceneChanged(EntitySceneOp op, IKEntity* entity)
{
	KRenderComponent* renderComponent = nullptr;
	KTransformComponent* transformComponent = nullptr;

	ASSERT_RESULT(entity->GetComponent(CT_RENDER, &renderComponent));
	ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));

	if (op == ESO_ADD)
	{
		const glm::mat4& transform = transformComponent->GetFinal();
		if (renderComponent->IsVirtualGeometry())
		{
			KVirtualGeometryResourceRef vg = renderComponent->GetVirtualGeometry();
			AddInstance(entity, transform, vg);
		}
		renderComponent->RegisterCallback(&m_OnRenderComponentChangedFunc);
	}
	if (op == ESO_REMOVE)
	{
		RemoveInstance(entity);
		renderComponent->UnRegisterCallback(&m_OnRenderComponentChangedFunc);
	}
	else if (op == ESO_TRANSFORM)
	{
		const glm::mat4& transform = transformComponent->GetFinal();
		TransformInstance(entity, transform);
	}
}

void KVirtualGeometryScene::OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init)
{
	IKEntity* entity = renderComponent->GetEntityHandle();
	if (!entity)
	{
		return;
	}

	if (init)
	{
		if (renderComponent->IsVirtualGeometry())
		{
			KTransformComponent* transformComponent = nullptr;
			ASSERT_RESULT(entity->GetComponent(CT_TRANSFORM, &transformComponent));
			const glm::mat4& transform = transformComponent->GetFinal();

			KVirtualGeometryResourceRef vg = ((KRenderComponent*)renderComponent)->GetVirtualGeometry();
			AddInstance(entity, transform, vg);
		}
	}
	else
	{
		RemoveInstance(entity);
	}
}

bool KVirtualGeometryScene::Init(IKRenderScene* scene, const KCamera* camera)
{
	UnInit();
	if (scene && camera)
	{
		m_Scene = scene;
		m_Camera = camera;
		m_PrevViewProj = m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix();
		{
			KRenderGlobal::RenderDevice->CreateUniformBuffer(m_GlobalDataBuffer);
			m_GlobalDataBuffer->InitMemory(sizeof(KVirtualGeometryGlobal), nullptr);
			m_GlobalDataBuffer->InitDevice();
			m_GlobalDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_GLOBAL_DATA);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_InstanceDataBuffer);
			m_InstanceDataBuffer->InitMemory(sizeof(KVirtualGeometryInstance), nullptr);
			m_InstanceDataBuffer->InitDevice(false);
			m_InstanceDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INSTANCE_DATA);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_QueueStateBuffer);
			m_QueueStateBuffer->InitMemory(sizeof(KVirtualGeometryQueueState), nullptr);
			m_QueueStateBuffer->InitDevice(false);
			m_QueueStateBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_QUEUE_STATE);

			std::vector<uint32_t> emptyCandidateNodeData;
			emptyCandidateNodeData.resize(MAX_CANDIDATE_NODE);
			memset(emptyCandidateNodeData.data(), -1, MAX_CANDIDATE_NODE * sizeof(uint32_t));

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_CandidateNodeBuffer);
			m_CandidateNodeBuffer->InitMemory(MAX_CANDIDATE_NODE * sizeof(uint32_t), emptyCandidateNodeData.data());
			m_CandidateNodeBuffer->InitDevice(false);
			m_CandidateNodeBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_CANDIDATE_NODE);

			std::vector<uint32_t> emptyBatchClusterData;
			emptyBatchClusterData.resize(MAX_CANDIDATE_CLUSTERS);
			memset(emptyBatchClusterData.data(), -1, MAX_CANDIDATE_CLUSTERS * sizeof(uint32_t));

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_CandidateClusterBuffer);
			m_CandidateClusterBuffer->InitMemory(MAX_CANDIDATE_CLUSTERS * sizeof(uint32_t), emptyBatchClusterData.data());
			m_CandidateClusterBuffer->InitDevice(false);
			m_CandidateClusterBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_CANDIDATE_CLUSTER);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_SelectedClusterBuffer);
			m_SelectedClusterBuffer->InitMemory(MAX_CANDIDATE_CLUSTERS * sizeof(uint32_t), emptyBatchClusterData.data());
			m_SelectedClusterBuffer->InitDevice(false);
			m_SelectedClusterBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_SELECTED_CLUSTER);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_ExtraDebugBuffer);
			m_ExtraDebugBuffer->InitMemory(MAX_CANDIDATE_CLUSTERS * sizeof(uint32_t), emptyBatchClusterData.data());
			m_ExtraDebugBuffer->InitDevice(false);
			m_ExtraDebugBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_BINDING_EXTRA_DEBUG_INFO);

			uint32_t indirectInfo[] = { 1, 1, 1 };
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_IndirectAgrsBuffer);
			m_IndirectAgrsBuffer->InitMemory(sizeof(indirectInfo), indirectInfo);
			m_IndirectAgrsBuffer->InitDevice(true);
			m_IndirectAgrsBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_ARGS);

			// { vertexCount, instanceCount, firstVertex, firstInstance }
			int32_t indirectDrawInfo[] = { 0, 0, 0, 0 };
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_IndirectDrawBuffer);
			m_IndirectDrawBuffer->InitMemory(sizeof(indirectDrawInfo), indirectDrawInfo);
			m_IndirectDrawBuffer->InitDevice(true);
			m_IndirectDrawBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_DRAW_ARGS);

			std::vector<glm::uvec4> emptyBinningBatchData;
			emptyBinningBatchData.resize(MAX_CANDIDATE_CLUSTERS);
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_BinningDataBuffer);
			m_BinningDataBuffer->InitMemory(emptyBinningBatchData.size() * sizeof(glm::uvec4), emptyBinningBatchData.data());
			m_BinningDataBuffer->InitDevice(false);
			m_BinningDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_BINNING_DATA);

			uint32_t emptyBinningHeader[] = { 0, 0, 0, 0 };
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_BinningHeaderBuffer);
			m_BinningHeaderBuffer->InitMemory(sizeof(emptyBinningHeader), emptyBinningHeader);
			m_BinningHeaderBuffer->InitDevice(false);
			m_BinningHeaderBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_BINNIIG_HEADER);
		}

		{
			KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitQueueStatePipeline);
			m_InitQueueStatePipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_OUT, true);
			m_InitQueueStatePipeline->Init("virtualgeometry/init.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_InstanceCullPipeline);
			m_InstanceCullPipeline->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
			m_InstanceCullPipeline->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);
			m_InstanceCullPipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
			m_InstanceCullPipeline->BindStorageBuffer(BINDING_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
			m_InstanceCullPipeline->BindStorageBuffer(BINDING_CLUSTER_BATCH, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer(), COMPUTE_RESOURCE_IN, true);
			m_InstanceCullPipeline->BindStorageBuffer(BINDING_CANDIDATE_NODE_BATCH, m_CandidateNodeBuffer, COMPUTE_RESOURCE_OUT, true);
			m_InstanceCullPipeline->Init("virtualgeometry/instance_cull.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitNodeCullArgsPipeline);
			m_InitNodeCullArgsPipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
			m_InitNodeCullArgsPipeline->BindStorageBuffer(BINDING_INDIRECT_ARGS, m_IndirectAgrsBuffer, COMPUTE_RESOURCE_OUT, true);
			m_InitNodeCullArgsPipeline->Init("virtualgeometry/init_node_cull_args.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitClusterCullArgsPipeline);
			m_InitClusterCullArgsPipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
			m_InitClusterCullArgsPipeline->BindStorageBuffer(BINDING_INDIRECT_ARGS, m_IndirectAgrsBuffer, COMPUTE_RESOURCE_OUT, true);
			m_InitClusterCullArgsPipeline->Init("virtualgeometry/init_cluster_cull_args.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_NodeCullPipeline);
			m_NodeCullPipeline->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
			m_NodeCullPipeline->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);
			m_NodeCullPipeline->BindStorageBuffer(BINDING_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
			m_NodeCullPipeline->BindStorageBuffer(BINDING_HIERARCHY, KRenderGlobal::VirtualGeometryManager.GetPackedHierarchyBuffer(), COMPUTE_RESOURCE_IN, true);
			m_NodeCullPipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
			m_NodeCullPipeline->BindStorageBuffer(BINDING_CANDIDATE_NODE_BATCH, m_CandidateNodeBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
			m_NodeCullPipeline->BindStorageBuffer(BINDING_CANDIDATE_CLUSTER_BATCH, m_CandidateClusterBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
			m_NodeCullPipeline->BindStorageBuffer(BINDING_EXTRA_DEBUG_INFO, m_ExtraDebugBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
			m_NodeCullPipeline->Init("virtualgeometry/node_cull.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_ClusterCullPipeline);
			m_ClusterCullPipeline->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
			m_ClusterCullPipeline->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);
			m_ClusterCullPipeline->BindStorageBuffer(BINDING_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
			m_ClusterCullPipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
			m_ClusterCullPipeline->BindStorageBuffer(BINDING_CLUSTER_BATCH, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer(), COMPUTE_RESOURCE_IN, true);
			m_ClusterCullPipeline->BindStorageBuffer(BINDING_CANDIDATE_CLUSTER_BATCH, m_CandidateClusterBuffer, COMPUTE_RESOURCE_IN, true);
			m_ClusterCullPipeline->BindStorageBuffer(BINDING_SELECTED_CLUSTER_BATCH, m_SelectedClusterBuffer, COMPUTE_RESOURCE_OUT, true);
			m_ClusterCullPipeline->BindStorageBuffer(BINDING_EXTRA_DEBUG_INFO, m_ExtraDebugBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
			m_ClusterCullPipeline->Init("virtualgeometry/cluster_cull.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_CalcDrawArgsPipeline);
			m_CalcDrawArgsPipeline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
			m_CalcDrawArgsPipeline->BindStorageBuffer(BINDING_INDIRECT_DRAW_ARGS, m_IndirectDrawBuffer, COMPUTE_RESOURCE_OUT, true);
			m_CalcDrawArgsPipeline->Init("virtualgeometry/calc_draw_args.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_InitBinningPipline);
			m_InitBinningPipline->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
			m_InitBinningPipline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
			m_InitBinningPipline->BindStorageBuffer(BINDING_INDIRECT_ARGS, m_IndirectAgrsBuffer, COMPUTE_RESOURCE_OUT, true);
			m_InitBinningPipline->BindStorageBuffer(BINDING_BINNING_HEADER, m_BinningHeaderBuffer, COMPUTE_RESOURCE_OUT, true);
			m_InitBinningPipline->Init("virtualgeometry/init_binning.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_BinningClassifyPipline);
			m_BinningClassifyPipline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
			m_BinningClassifyPipline->BindStorageBuffer(BINDING_SELECTED_CLUSTER_BATCH, m_SelectedClusterBuffer, COMPUTE_RESOURCE_IN, true);
			m_BinningClassifyPipline->BindStorageBuffer(BINDING_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
			m_BinningClassifyPipline->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);;
			m_BinningClassifyPipline->BindStorageBuffer(BINDING_CLUSTER_BATCH, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer(), COMPUTE_RESOURCE_IN, true);
			m_BinningClassifyPipline->BindStorageBuffer(BINDING_BINNING_HEADER, m_BinningHeaderBuffer, COMPUTE_RESOURCE_OUT, true);
			m_BinningClassifyPipline->Init("virtualgeometry/binning_classify.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_BinningAllocatePipline);
			m_BinningAllocatePipline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
			m_BinningAllocatePipline->BindUniformBuffer(BINDING_GLOBAL_DATA, m_GlobalDataBuffer);
			m_BinningAllocatePipline->BindStorageBuffer(BINDING_BINNING_HEADER, m_BinningHeaderBuffer, COMPUTE_RESOURCE_OUT, true);
			m_BinningAllocatePipline->BindStorageBuffer(BINDING_INDIRECT_DRAW_ARGS, m_IndirectDrawBuffer, COMPUTE_RESOURCE_OUT, true);
			m_BinningAllocatePipline->Init("virtualgeometry/binning_allocate.comp");

			KRenderGlobal::RenderDevice->CreateComputePipeline(m_BinningScatterPipline);
			m_BinningScatterPipline->BindStorageBuffer(BINDING_QUEUE_STATE, m_QueueStateBuffer, COMPUTE_RESOURCE_IN, true);
			m_BinningScatterPipline->BindStorageBuffer(BINDING_SELECTED_CLUSTER_BATCH, m_SelectedClusterBuffer, COMPUTE_RESOURCE_IN, true);
			m_BinningScatterPipline->BindStorageBuffer(BINDING_INSTANCE_DATA, m_InstanceDataBuffer, COMPUTE_RESOURCE_IN, true);
			m_BinningScatterPipline->BindStorageBuffer(BINDING_RESOURCE, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer(), COMPUTE_RESOURCE_IN, true);;
			m_BinningScatterPipline->BindStorageBuffer(BINDING_CLUSTER_BATCH, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer(), COMPUTE_RESOURCE_IN, true);
			m_BinningScatterPipline->BindStorageBuffer(BINDING_BINNING_HEADER, m_BinningHeaderBuffer, COMPUTE_RESOURCE_OUT, true);
			m_BinningScatterPipline->BindStorageBuffer(BINDING_BINNING_DATA, m_BinningDataBuffer, COMPUTE_RESOURCE_OUT, true);
			m_BinningScatterPipline->BindStorageBuffer(BINDING_INDIRECT_DRAW_ARGS, m_IndirectDrawBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, true);
			m_BinningScatterPipline->Init("virtualgeometry/binning_scatter.comp");
		}

		{
			KShaderCompileEnvironment env;
			KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "virtualgeometry/vg_basepass.vert", env, m_BasePassVertexShader, false);
		}

		{
			KShaderCompileEnvironment env;
			KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "virtualgeometry/vg_debug.vert", env, m_DebugVertexShader, false);
			KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "virtualgeometry/vg_debug.frag", env, m_DebugFragmentShader, false);

			KRenderGlobal::RenderDevice->CreatePipeline(m_DebugPipeline);
			m_DebugPipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			m_DebugPipeline->SetBlendEnable(false);
			m_DebugPipeline->SetCullMode(CM_BACK);
			m_DebugPipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			m_DebugPipeline->SetPolygonMode(PM_FILL);
			m_DebugPipeline->SetColorWrite(true, true, true, true);
			m_DebugPipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			m_DebugPipeline->SetShader(ST_VERTEX, m_DebugVertexShader.Get());
			m_DebugPipeline->SetShader(ST_FRAGMENT, m_DebugFragmentShader.Get());

			m_DebugPipeline->SetStorageBuffer(BINDING_SELECTED_CLUSTER_BATCH, ST_VERTEX, m_SelectedClusterBuffer);
			m_DebugPipeline->SetStorageBuffer(BINDING_INSTANCE_DATA, ST_VERTEX, m_InstanceDataBuffer);
			m_DebugPipeline->SetStorageBuffer(BINDING_RESOURCE, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer());
			m_DebugPipeline->SetStorageBuffer(BINDING_CLUSTER_BATCH, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer());

			m_DebugPipeline->SetStorageBuffer(BINDING_CLUSTER_VERTEX_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterVertexStorageBuffer());
			m_DebugPipeline->SetStorageBuffer(BINDING_CLUSTER_INDEX_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterIndexStorageBuffer());

			m_DebugPipeline->SetStorageBuffer(BINDING_BINNING_DATA, ST_VERTEX, m_BinningDataBuffer);
			m_DebugPipeline->SetStorageBuffer(BINDING_BINNING_HEADER, ST_VERTEX, m_BinningHeaderBuffer);

			m_DebugPipeline->SetConstantBuffer(BINDING_GLOBAL_DATA, ST_VERTEX, m_GlobalDataBuffer);

			m_DebugPipeline->Init();
		}

		std::vector<IKEntity*> entites;
		scene->GetAllEntities(entites);

		for (IKEntity*& entity : entites)
		{
			OnSceneChanged(ESO_ADD, entity);
		}

		m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);

		return true;
	}
	return false;
}

bool KVirtualGeometryScene::UnInit()
{
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
		m_Scene = nullptr;
	}

	m_Camera = nullptr;

	SAFE_UNINIT(m_GlobalDataBuffer);
	SAFE_UNINIT(m_InstanceDataBuffer);
	SAFE_UNINIT(m_QueueStateBuffer);
	SAFE_UNINIT(m_CandidateNodeBuffer);
	SAFE_UNINIT(m_CandidateClusterBuffer);
	SAFE_UNINIT(m_IndirectAgrsBuffer);
	SAFE_UNINIT(m_SelectedClusterBuffer);
	SAFE_UNINIT(m_ExtraDebugBuffer);
	SAFE_UNINIT(m_IndirectDrawBuffer);
	SAFE_UNINIT(m_BinningDataBuffer);
	SAFE_UNINIT(m_BinningHeaderBuffer);

	SAFE_UNINIT(m_InitQueueStatePipeline);
	SAFE_UNINIT(m_InstanceCullPipeline);
	SAFE_UNINIT(m_InitNodeCullArgsPipeline);
	SAFE_UNINIT(m_InitClusterCullArgsPipeline);
	SAFE_UNINIT(m_NodeCullPipeline);
	SAFE_UNINIT(m_ClusterCullPipeline);
	SAFE_UNINIT(m_CalcDrawArgsPipeline);
	SAFE_UNINIT(m_InitBinningPipline);
	SAFE_UNINIT(m_BinningClassifyPipline);
	SAFE_UNINIT(m_BinningAllocatePipline);
	SAFE_UNINIT(m_BinningScatterPipline);

	SAFE_UNINIT(m_DebugPipeline);

	m_DebugVertexShader.Release();
	m_DebugFragmentShader.Release();

	m_InstanceMap.clear();
	m_Instances.clear();
	m_LastInstanceData.clear();
	m_BinningMaterials.clear();

	m_BasePassVertexShader.Release();
	m_BasePassFragmentShaders.clear();

	SAFE_UNINIT_CONTAINER(m_BinningPipelines);

	return true;
}

bool KVirtualGeometryScene::UpdateInstanceData()
{
	void* pWrite = nullptr;

	if (m_GlobalDataBuffer)
	{
		KVirtualGeometryGlobal globalData;

		globalData.worldToClip = m_Camera ? (m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix()) : glm::mat4(1);
		globalData.prevWorldToClip = m_PrevViewProj;
		globalData.worldToView = m_Camera ? m_Camera->GetViewMatrix() : glm::mat4(1);
		globalData.misc.x = m_Camera ? m_Camera->GetNear() : 0.0f;
		globalData.misc.y = m_Camera ? m_Camera->GetAspect() : 1.0f;

		globalData.miscs2.x = (uint32_t)m_Instances.size();
		globalData.miscs2.y = (uint32_t)m_BinningMaterials.size();

		size_t sceneWidth = 0;
		size_t sceneHeight = 0;
		KRenderGlobal::GBuffer.GetSceneColor()->GetSize(sceneWidth, sceneHeight);
		globalData.misc.z = m_Camera ? (sceneHeight / m_Camera->GetHeight()) : 1.0f;

		m_GlobalDataBuffer->Map(&pWrite);
		memcpy(pWrite, &globalData, sizeof(globalData));
		m_GlobalDataBuffer->UnMap();
		pWrite = nullptr;

		m_PrevViewProj = globalData.worldToClip;
	}

	if (!m_InstanceDataBuffer || !m_IndirectDrawBuffer)
	{
		return false;
	}

	std::vector<KVirtualGeometryInstance> instanceData;
	instanceData.resize(m_Instances.size());

	const std::vector<KMaterialRef>& allMaterials = KRenderGlobal::VirtualGeometryManager.GetAllMaterials();
	std::unordered_map<uint32_t, uint32_t> materialIndexToBinningIndex;

	std::vector<KMaterialRef> binningMaterials;

	for (size_t i = 0; i < m_Instances.size(); ++i)
	{
		KVirtualGeometryResourceRef resource = m_Instances[i]->resource;
		instanceData[i].resourceIndex = resource->resourceIndex;
		instanceData[i].prevTransform = m_Instances[i]->prevTransform;
		instanceData[i].transform = m_Instances[i]->transform;

		uint32_t materialBaseIndex = resource->materialBaseIndex;
		uint32_t binningBaseIndex = 0;

		auto it = materialIndexToBinningIndex.find(materialBaseIndex);
		if (it != materialIndexToBinningIndex.end())
		{
			binningBaseIndex = it->second;
		}
		else
		{
			binningBaseIndex = (uint32_t)binningMaterials.size();
			for (uint32_t localMaterialIndex = 0; localMaterialIndex < resource->materialNum; ++localMaterialIndex)
			{
				uint32_t materialIndex = materialBaseIndex + localMaterialIndex;
				uint32_t binningIndex = binningBaseIndex + localMaterialIndex;
				binningMaterials.push_back(allMaterials[materialIndex]);
				materialIndexToBinningIndex[materialIndex] = binningIndex;
			}
		}

		instanceData[i].binningBaseIndex = binningBaseIndex;

		m_Instances[i]->prevTransform = m_Instances[i]->transform;
	}

	if (instanceData.size() == m_LastInstanceData.size())
	{
		if (memcmp(instanceData.data(), m_LastInstanceData.data(), m_LastInstanceData.size() * sizeof(KVirtualGeometryInstance)) == 0)
		{
			return true;
		}
	}

	m_BinningMaterials = std::move(binningMaterials);

	// TODO
	SAFE_UNINIT_CONTAINER(m_BinningPipelines);
	m_BasePassFragmentShaders.clear();
	m_BinningPipelines.resize(m_BinningMaterials.size());
	m_BasePassFragmentShaders.resize(m_BinningMaterials.size());

	for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
	{
		IKPipelinePtr& pipeline = m_BinningPipelines[i];
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		KShaderCompileEnvironment env;
		env.includes.push_back({"material_generate_code.h", m_BinningMaterials[i]->GetMaterialGeneratedCode()});
		
		const IKMaterialTextureBinding* textureBinding = m_BinningMaterials[i]->GetTextureBinding().get();
		uint8_t numSlot = textureBinding->GetNumSlot();
		for (uint32_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
		{
			env.macros.push_back({ PERMUTATING_MACRO[MATERIAL_TEXTURE_BINDING_MACRO_INDEX[i]].macro, textureBinding->GetTexture(i) ? "1" : "0" });
		}

		KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "virtualgeometry/vg_basepass.frag", env, m_BasePassFragmentShaders[i], false);
		IKShaderPtr vsShader = m_BasePassVertexShader.Get();
		IKShaderPtr fsShader = m_BasePassFragmentShaders[i].Get();

		pipeline->SetVertexBinding(nullptr, 0);
		pipeline->SetShader(ST_VERTEX, vsShader);
		pipeline->SetShader(ST_FRAGMENT, fsShader);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetStorageBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_SELECTED_CLUSTER_BATCH, ST_VERTEX, m_SelectedClusterBuffer);
		pipeline->SetStorageBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_INSTANCE_DATA, ST_VERTEX, m_InstanceDataBuffer);
		pipeline->SetStorageBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_RESOURCE, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetResourceBuffer());
		pipeline->SetStorageBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_CLUSTER_BATCH, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterBatchBuffer());

		pipeline->SetStorageBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_CLUSTER_VERTEX_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterVertexStorageBuffer());
		pipeline->SetStorageBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_CLUSTER_INDEX_BUFFER, ST_VERTEX, KRenderGlobal::VirtualGeometryManager.GetClusterIndexStorageBuffer());

		pipeline->SetStorageBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_BINNING_DATA, ST_VERTEX, m_BinningDataBuffer);
		pipeline->SetStorageBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_BINNING_HEADER, ST_VERTEX, m_BinningHeaderBuffer);

		pipeline->SetConstantBuffer(MAX_MATERIAL_TEXTURE_BINDING + BINDING_GLOBAL_DATA, ST_VERTEX | ST_FRAGMENT, m_GlobalDataBuffer);

		for (uint8_t i = 0; i < numSlot; ++i)
		{
			IKTexturePtr texture = textureBinding->GetTexture(i);
			IKSamplerPtr sampler = textureBinding->GetSampler(i);
			if (texture && sampler)
			{
				pipeline->SetSampler(SHADER_BINDING_TEXTURE0 + i, texture->GetFrameBuffer(), sampler, true);
			}
		}

		pipeline->Init();
	}

	{
		int32_t indirectDrawInfo[] = { 0, 0, 0, 0 };

		size_t binningCount = m_BinningMaterials.size();// +1;
		size_t dataSize = sizeof(indirectDrawInfo) * binningCount;
		size_t targetBufferSize = sizeof(indirectDrawInfo) * std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(binningCount));

		if (m_IndirectDrawBuffer->GetBufferSize() != targetBufferSize)
		{
			KRenderGlobal::RenderDevice->Wait();
			m_IndirectDrawBuffer->UnInit();
			m_IndirectDrawBuffer->InitMemory(targetBufferSize, nullptr);
			m_IndirectDrawBuffer->InitDevice(true);
			m_IndirectDrawBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_DRAW_ARGS);
		}

		if (dataSize)
		{
			m_InstanceDataBuffer->Map(&pWrite);
			for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
			{
				memcpy(pWrite, indirectDrawInfo, sizeof(indirectDrawInfo));
				pWrite = POINTER_OFFSET(pWrite, sizeof(indirectDrawInfo));
			}
			m_InstanceDataBuffer->UnMap();
			pWrite = nullptr;
		}
	}

	{
		uint32_t binningHeader[] = { 0, 0, 0, 0 };

		size_t dataSize = sizeof(binningHeader) * m_BinningMaterials.size();
		size_t targetBufferSize = sizeof(binningHeader) * std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(m_BinningMaterials.size()));

		if (m_BinningHeaderBuffer->GetBufferSize() != targetBufferSize)
		{
			KRenderGlobal::RenderDevice->Wait();
			m_BinningHeaderBuffer->UnInit();
			m_BinningHeaderBuffer->InitMemory(targetBufferSize, nullptr);
			m_BinningHeaderBuffer->InitDevice(false);
			m_BinningHeaderBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_BINNIIG_HEADER);
		}

		if (dataSize)
		{
			m_BinningHeaderBuffer->Map(&pWrite);
			for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
			{
				memcpy(pWrite, binningHeader, sizeof(binningHeader));
				pWrite = POINTER_OFFSET(pWrite, sizeof(binningHeader));
			}
			m_BinningHeaderBuffer->UnMap();
			pWrite = nullptr;
		}
	}

	{
		size_t dataSize = sizeof(KVirtualGeometryInstance) * instanceData.size();
		size_t targetBufferSize = sizeof(KVirtualGeometryInstance) * std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(instanceData.size()));

		if (m_InstanceDataBuffer->GetBufferSize() != targetBufferSize)
		{
			KRenderGlobal::RenderDevice->Wait();

			m_InstanceDataBuffer->UnInit();
			m_InstanceDataBuffer->InitMemory(targetBufferSize, nullptr);
			m_InstanceDataBuffer->InitDevice(false);
			m_InstanceDataBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INSTANCE_DATA);
		}

		if (dataSize)
		{
			m_InstanceDataBuffer->Map(&pWrite);
			memcpy(pWrite, instanceData.data(), dataSize);
			m_InstanceDataBuffer->UnMap();
			pWrite = nullptr;
		}

		m_LastInstanceData = std::move(instanceData);
	}

	return true;
}

bool KVirtualGeometryScene::Execute(IKCommandBufferPtr primaryBuffer)
{
	UpdateInstanceData();

	primaryBuffer->BeginDebugMarker("VirtualGeometry", glm::vec4(1));
	if (m_InitQueueStatePipeline)
	{
		{
			primaryBuffer->BeginDebugMarker("VirtualGeometry_Init", glm::vec4(1));
			m_InitQueueStatePipeline->Execute(primaryBuffer, 1, 1, 1, nullptr);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("VirtualGeometry_InstanceCull", glm::vec4(1));
			uint32_t groupNum = 0;
			groupNum = ((uint32_t)m_Instances.size() + VG_GROUP_SIZE - 1) / VG_GROUP_SIZE;
			m_InstanceCullPipeline->Execute(primaryBuffer, groupNum, 1, 1, nullptr);
			primaryBuffer->EndDebugMarker();
		}

		for (uint32_t level = 0; level < 12; ++level)
		{
			primaryBuffer->BeginDebugMarker(("VirtualGeometry_InitNodeCullArgs_" + std::to_string(level)).c_str(), glm::vec4(1));
			m_InitNodeCullArgsPipeline->Execute(primaryBuffer, 1, 1, 1, nullptr);
			primaryBuffer->EndDebugMarker();

			primaryBuffer->BeginDebugMarker(("VirtualGeometry_NodeCull_" + std::to_string(level)).c_str(), glm::vec4(1));
			m_NodeCullPipeline->ExecuteIndirect(primaryBuffer, m_IndirectAgrsBuffer, nullptr);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("VirtualGeometry_InitNodeCulusterArgs", glm::vec4(1));
			m_InitClusterCullArgsPipeline->Execute(primaryBuffer, 1, 1, 1, nullptr);
			primaryBuffer->EndDebugMarker();

			primaryBuffer->BeginDebugMarker("VirtualGeometry_ClusterCull", glm::vec4(1));
			m_ClusterCullPipeline->ExecuteIndirect(primaryBuffer, m_IndirectAgrsBuffer, nullptr);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("VirtualGeometry_CalcDrawArgs", glm::vec4(1));
			m_CalcDrawArgsPipeline->ExecuteIndirect(primaryBuffer, m_IndirectAgrsBuffer, nullptr);
			primaryBuffer->EndDebugMarker();
		}

		{
			primaryBuffer->BeginDebugMarker("VirtualGeometry_Binning", glm::vec4(1));
			{
				uint32_t numGroup = (uint32_t)(m_BinningMaterials.size() + VG_GROUP_SIZE - 1) / VG_GROUP_SIZE;
				{
					primaryBuffer->BeginDebugMarker("VirtualGeometry_InitBinning", glm::vec4(1));
					m_InitBinningPipline->Execute(primaryBuffer, numGroup, 1, 1, nullptr);
					primaryBuffer->EndDebugMarker();

					primaryBuffer->BeginDebugMarker("VirtualGeometry_BinningClassify", glm::vec4(1));
					m_BinningClassifyPipline->ExecuteIndirect(primaryBuffer, m_IndirectAgrsBuffer, nullptr);
					primaryBuffer->EndDebugMarker();

					primaryBuffer->BeginDebugMarker("VirtualGeometry_BinningAllocate", glm::vec4(1));
					m_BinningAllocatePipline->Execute(primaryBuffer, numGroup, 1, 1, nullptr);
					primaryBuffer->EndDebugMarker();

					primaryBuffer->BeginDebugMarker("VirtualGeometry_BinningScatter", glm::vec4(1));
					m_BinningScatterPipline->ExecuteIndirect(primaryBuffer, m_IndirectAgrsBuffer, nullptr);
					primaryBuffer->EndDebugMarker();
				}
			}
			primaryBuffer->EndDebugMarker();
		}
	}
	primaryBuffer->EndDebugMarker();
	return true;
}

bool KVirtualGeometryScene::BasePass(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker("VirtualGeometry_BasePass", glm::vec4(1));

	for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
	{
		KRenderCommand command;

		if (!KRenderUtil::AssignShadingParameter(command, m_BinningMaterials[i]))
		{
			continue;
		}

		KVirtualGeometryMaterial material;
		material.miscs3.x = (uint32_t)i;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = BINDING_MATERIAL_DATA + MAX_MATERIAL_TEXTURE_BINDING;
		objectUsage.range = sizeof(material);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&material, objectUsage);

		command.dynamicConstantUsages.push_back(objectUsage);

		command.pipeline = m_BinningPipelines[i];
		command.indirectArgsBuffer = m_IndirectDrawBuffer;
		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = false;
		command.indirectDraw = true;
		command.indrectOffset = (uint32_t)i;

		primaryBuffer->Render(command);
	}

	primaryBuffer->EndDebugMarker();

	return true;
}

bool KVirtualGeometryScene::DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
{
	primaryBuffer->BeginDebugMarker("VirtualGeometry_DebugRender", glm::vec4(1));
#if 0
	for (size_t i = 0; i < m_BinningMaterials.size(); ++i)
	{
		KRenderCommand command;

		command.pipeline = m_DebugPipeline;
		command.indirectArgsBuffer = m_IndirectDrawBuffer;
		command.pipeline->GetHandle(renderPass, command.pipelineHandle);
		command.indexDraw = false;
		command.indirectDraw = true;
		command.indrectOffset = (uint32_t)i;

		KVirtualGeometryMaterial material;
		material.miscs3.x = (uint32_t)i;
		command.objectUsage.binding = BINDING_MATERIAL_DATA;
		command.objectUsage.range = sizeof(material);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&material, command.objectUsage);

		primaryBuffer->Render(command);
	}
#endif
	primaryBuffer->EndDebugMarker();
	return true;
}

bool KVirtualGeometryScene::ReloadShader()
{
	if (m_InitQueueStatePipeline)
	{
		m_InitQueueStatePipeline->Reload();
	}
	if (m_InstanceCullPipeline)
	{
		m_InstanceCullPipeline->Reload();
	}
	if (m_InitNodeCullArgsPipeline)
	{
		m_InitNodeCullArgsPipeline->Reload();
	}
	if (m_InitClusterCullArgsPipeline)
	{
		m_InitClusterCullArgsPipeline->Reload();
	}
	if (m_NodeCullPipeline)
	{
		m_NodeCullPipeline->Reload();
	}
	if (m_ClusterCullPipeline)
	{
		m_ClusterCullPipeline->Reload();
	}
	if (m_CalcDrawArgsPipeline)
	{
		m_CalcDrawArgsPipeline->Reload();
	}
	if (m_BasePassVertexShader)
	{
		m_BasePassVertexShader->Reload();
	}
	if (m_DebugVertexShader)
	{
		m_DebugVertexShader->Reload();
	}
	if (m_DebugFragmentShader)
	{
		m_DebugFragmentShader->Reload();
	}
	if (m_InitBinningPipline)
	{
		m_InitBinningPipline->Reload();
	}
	if (m_BinningClassifyPipline)
	{
		m_BinningClassifyPipline->Reload();
	}
	if (m_BinningAllocatePipline)
	{
		m_BinningAllocatePipline->Reload();
	}
	if (m_BinningScatterPipline)
	{
		m_BinningScatterPipline->Reload();
	}
	if (m_DebugPipeline)
	{
		m_DebugPipeline->Reload();
	}
	return true;
}

KVirtualGeometryScene::InstancePtr KVirtualGeometryScene::GetOrCreateInstance(IKEntity* entity)
{
	auto it = m_InstanceMap.find(entity);
	if (it != m_InstanceMap.end())
	{
		return it->second;
	}
	else
	{
		InstancePtr Instance(KNEW Instance());
		Instance->index = (uint32_t)m_Instances.size();
		Instance->transform = glm::mat4(0.0f);
		m_Instances.push_back(Instance);
		m_InstanceMap.insert({ entity, Instance });
		return Instance;
	}
}

bool KVirtualGeometryScene::AddInstance(IKEntity* entity, const glm::mat4& transform, KVirtualGeometryResourceRef resource)
{
	if (entity)
	{
		InstancePtr Instance = GetOrCreateInstance(entity);
		Instance->prevTransform = transform;
		Instance->transform = transform;
		Instance->resource = resource;
		return true;
	}
	return false;
}

bool KVirtualGeometryScene::TransformInstance(IKEntity* entity, const glm::mat4& transform)
{
	if (entity)
	{
		InstancePtr Instance = GetOrCreateInstance(entity);
		Instance->transform = transform;
		return true;
	}
	return false;
}

bool KVirtualGeometryScene::RemoveInstance(IKEntity* entity)
{
	auto it = m_InstanceMap.find(entity);
	if (it != m_InstanceMap.end())
	{
		uint32_t index = it->second->index;
		assert(index < m_Instances.size());
		for (uint32_t i = index + 1; i < m_Instances.size(); ++i)
		{
			--m_Instances[i]->index;
		}
		m_InstanceMap.erase(it);
		m_Instances.erase(m_Instances.begin() + index);
	}
	return true;
}