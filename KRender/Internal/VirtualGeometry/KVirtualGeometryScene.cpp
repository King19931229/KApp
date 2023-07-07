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
		m_Name = "VirtualGeometrySceneInstanceData";
		KRenderGlobal::RenderDevice->CreateStorageBuffer(m_InstanceDataBuffer);
		m_InstanceDataBuffer->InitMemory(1, nullptr);
		m_InstanceDataBuffer->InitDevice(false);
		m_InstanceDataBuffer->SetDebugName(m_Name.c_str());

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
	SAFE_UNINIT(m_InstanceDataBuffer);
	m_InstanceMap.clear();
	m_Instances.clear();

	return true;
}

bool KVirtualGeometryScene::UpdateInstanceData()
{
	if (!m_InstanceDataBuffer)
	{
		return false;
	}

	std::vector<InstanceBufferData> instanceData;
	instanceData.resize(m_Instances.size());

	for (size_t i = 0; i < m_Instances.size(); ++i)
	{
		KVirtualGeometryResourceRef resource = m_Instances[i]->resource;
		instanceData[i].resourceIndex = resource->resourceIndex;
		instanceData[i].transform = m_Instances[i]->transform;
	}

	size_t dataSize = sizeof(InstanceBufferData) * instanceData.size();
	size_t targetBufferSize = std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(dataSize));

	if (m_InstanceDataBuffer->GetBufferSize() != targetBufferSize)
	{
		KRenderGlobal::RenderDevice->Wait();

		m_InstanceDataBuffer->UnInit();
		m_InstanceDataBuffer->InitMemory(targetBufferSize, nullptr);
		m_InstanceDataBuffer->InitDevice(false);
		m_InstanceDataBuffer->SetDebugName(m_Name.c_str());
	}

	if (dataSize)
	{
		void* pWrite = nullptr;
		m_InstanceDataBuffer->Map(&pWrite);
		memcpy(pWrite, instanceData.data(), dataSize);
		m_InstanceDataBuffer->UnMap();
	}

	return true;
}

bool KVirtualGeometryScene::Update()
{
	UpdateInstanceData();
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