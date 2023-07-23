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

			uint32_t indirectinfo[] = { 1, 1, 1 };
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_IndirectAgrsBuffer);
			m_IndirectAgrsBuffer->InitMemory(sizeof(indirectinfo), indirectinfo);
			m_IndirectAgrsBuffer->InitDevice(true);
			m_IndirectAgrsBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_INDIRECT_ARGS);
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

	SAFE_UNINIT(m_InitQueueStatePipeline);
	SAFE_UNINIT(m_InstanceCullPipeline);
	SAFE_UNINIT(m_InitNodeCullArgsPipeline);
	SAFE_UNINIT(m_InitClusterCullArgsPipeline);
	SAFE_UNINIT(m_NodeCullPipeline);
	SAFE_UNINIT(m_ClusterCullPipeline);

	m_InstanceMap.clear();
	m_Instances.clear();
	m_LastInstanceData.clear();

	return true;
}

bool KVirtualGeometryScene::UpdateInstanceData()
{
	void* pWrite = nullptr;

	if (m_GlobalDataBuffer)
	{
		KVirtualGeometryGlobal globalData;
		globalData.numInstance = (uint32_t)m_Instances.size();
		globalData.worldToClip = m_Camera ? (m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix()) : glm::mat4(1);
		globalData.worldToView = m_Camera ? m_Camera->GetViewMatrix() : glm::mat4(1);
		globalData.misc.x = m_Camera ? m_Camera->GetNear() : 0.0f;
		globalData.misc.y = m_Camera ? m_Camera->GetAspect() : 1.0f;
		m_GlobalDataBuffer->Map(&pWrite);
		memcpy(pWrite, &globalData, sizeof(globalData));
		m_GlobalDataBuffer->UnMap();
		pWrite = nullptr;
	}

	if (!m_InstanceDataBuffer)
	{
		return false;
	}

	std::vector<KVirtualGeometryInstance> instanceData;
	instanceData.resize(m_Instances.size());

	for (size_t i = 0; i < m_Instances.size(); ++i)
	{
		KVirtualGeometryResourceRef resource = m_Instances[i]->resource;
		instanceData[i].resourceIndex = resource->resourceIndex;
		instanceData[i].transform = m_Instances[i]->transform;
	}

	if (instanceData.size() == m_LastInstanceData.size())
	{
		if (memcmp(instanceData.data(), m_LastInstanceData.data(), m_LastInstanceData.size() * sizeof(KVirtualGeometryInstance)) == 0)
		{
			return true;
		}
	}

	size_t dataSize = sizeof(KVirtualGeometryInstance) * instanceData.size();
	size_t targetBufferSize = std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(dataSize));

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

		for(uint32_t level = 0; level < 12; ++level)
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
	}
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