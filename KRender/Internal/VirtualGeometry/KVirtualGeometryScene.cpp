#include "KVirtualGeometryScene.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KMath.h"

KVirtualGeometryScene::KVirtualGeometryScene()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
	, m_IDCounter(0)
{
	m_OnSceneChangedFunc = std::bind(&KVirtualGeometryScene::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KVirtualGeometryScene::~KVirtualGeometryScene()
{
	assert(m_Instances.empty());
}

void KVirtualGeometryScene::OnSceneChanged(EntitySceneOp op, IKEntity* entity)
{

}

bool KVirtualGeometryScene::Init(IKRenderScene* scene)
{
	UnInit();
	if (scene)
	{
		m_Scene = scene;
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
	m_UnusedIDS.clear();
	m_IDCounter = 0;

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

void KVirtualGeometryScene::RecycleID(KVirtualGeometrySceneID ID)
{
	m_UnusedIDS.push_back(ID);
}

KVirtualGeometrySceneID KVirtualGeometryScene::ObtainID()
{
	if (m_UnusedIDS.empty())
	{
		return m_IDCounter++;
	}
	else
	{
		KVirtualGeometrySceneID ID = m_UnusedIDS.front();
		m_UnusedIDS.pop_front();
		return ID;
	}
}

bool KVirtualGeometryScene::Update()
{
	UpdateInstanceData();
	return true;
}

bool KVirtualGeometryScene::AddInstance(const glm::mat4& transform, KVirtualGeometryResourceRef resource, KVirtualGeometrySceneID& ID)
{
	InstancePtr Instance(KNEW Instance());
	Instance->index = (uint32_t)m_Instances.size();
	Instance->transform = transform;
	m_Instances.push_back(Instance);
	m_InstanceMap.insert({ ID, Instance });
	return true;
}

bool KVirtualGeometryScene::RemoveInstance(KVirtualGeometrySceneID ID)
{
	auto it = m_InstanceMap.find(ID);
	if (it != m_InstanceMap.end())
	{
		uint32_t index = it->second->index;
		assert(index < m_Instances.size());
		for (uint32_t i = index + 1; i <= m_Instances.size(); ++i)
		{
			--m_Instances[i]->index;
		}
		m_InstanceMap.erase(it);
		m_Instances.erase(m_Instances.begin() + index);
		RecycleID(ID);
	}
	return true;
}