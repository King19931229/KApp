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
			m_QueueStateBuffer->InitMemory(sizeof(QueueState), nullptr);
			m_QueueStateBuffer->InitDevice(false);
			m_QueueStateBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_QUEUE_STATE);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_CandidateNodeBuffer);
			m_CandidateNodeBuffer->InitMemory(sizeof(glm::uvec4) * MAX_CANDIDATE_NODE, nullptr);
			m_CandidateNodeBuffer->InitDevice(false);
			m_CandidateNodeBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_CANDIDATE_NODE);

			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_CandidateClusterBuffer);
			m_CandidateClusterBuffer->InitMemory(sizeof(glm::uvec4) * MAX_CANDIDATE_CLUSTERS, nullptr);
			m_CandidateClusterBuffer->InitDevice(false);
			m_CandidateClusterBuffer->SetDebugName(VIRTUAL_GEOMETRY_SCENE_CANDIDATE_NODE);
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

	SAFE_UNINIT(m_InitQueueStatePipeline);
	SAFE_UNINIT(m_InstanceCullPipeline);

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
		globalData.worldToClip = m_Camera ? (m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix()) : glm::mat4(0);
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