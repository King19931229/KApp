#include "KVirtualGeometryManager.h"
#include "KVirtualGeometryScene.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KMath.h"

KVirtualGeometryStorageBuffer::KVirtualGeometryStorageBuffer()
	: m_Size(0)
	, m_Buffer(nullptr)
{
}

KVirtualGeometryStorageBuffer::~KVirtualGeometryStorageBuffer()
{
	assert(!m_Buffer);
}

bool KVirtualGeometryStorageBuffer::Init(const char* name, size_t initialSize)
{
	UnInit();
	m_Name = name;
	KRenderGlobal::RenderDevice->CreateStorageBuffer(m_Buffer);
	m_Buffer->InitMemory(initialSize, nullptr);
	m_Buffer->InitDevice(false);
	m_Buffer->SetDebugName(m_Name.c_str());
	return true;
}

bool KVirtualGeometryStorageBuffer::UnInit()
{
	m_Size = 0;
	SAFE_UNINIT(m_Buffer);
	return true;
}

bool KVirtualGeometryStorageBuffer::Remove(size_t offset, size_t size)
{
	if (m_Buffer == nullptr)
	{
		return false;
	}

	if (offset + size <= m_Size)
	{
		std::vector<unsigned char> bufferData;
		bufferData.resize(m_Size);
		m_Buffer->Read(bufferData.data());

		size_t newBufferSize = std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(m_Size - size));

		if (newBufferSize < m_Buffer->GetBufferSize())
		{
			SAFE_UNINIT(m_Buffer);
			KRenderGlobal::RenderDevice->CreateStorageBuffer(m_Buffer);
			m_Buffer->InitMemory(newBufferSize, nullptr);
			m_Buffer->InitDevice(false);
			m_Buffer->SetDebugName(m_Name.c_str());
		}

		void* pWrite = nullptr;
		m_Buffer->Map(&pWrite);
		memcpy(POINTER_OFFSET(pWrite, 0), bufferData.data(), offset);
		memcpy(POINTER_OFFSET(pWrite, offset), bufferData.data() + offset + size, m_Size - size - offset);
		m_Buffer->UnMap();

		m_Size -= size;

		return true;
	}
	else
	{
		return false;
	}
}

bool KVirtualGeometryStorageBuffer::Append(size_t size, void* pData)
{
	if (m_Buffer == nullptr)
	{
		return false;
	}

	size_t newBufferSize = std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(m_Size + size));

	if (newBufferSize > m_Buffer->GetBufferSize())
	{
		KRenderGlobal::RenderDevice->Wait();

		m_Buffer->UnInit();
		m_Buffer->InitMemory(newBufferSize, nullptr);
		m_Buffer->InitDevice(false);
		m_Buffer->SetDebugName(m_Name.c_str());
	}

	void* pWrite = nullptr;
	m_Buffer->Map(&pWrite);
	memcpy(POINTER_OFFSET(pWrite, m_Size), pData, size);
	m_Buffer->UnMap();

	m_Size += size;

	return true;
}

KVirtualGeometryManager::KVirtualGeometryManager()
{
}

KVirtualGeometryManager::~KVirtualGeometryManager()
{
	assert(m_GeometryMap.empty());
}

bool KVirtualGeometryManager::Init()
{
	UnInit();

	m_PackedHierarchyBuffer.Init("VirtualGeometryPackedHierarchy", sizeof(glm::uvec4));
	m_ClusterBatchBuffer.Init("VirtualGeometryClusterBatch", sizeof(glm::uvec4));
	m_ClusterVertexStorageBuffer.Init("VirtualGeometryVertexStorage", sizeof(float) * KMeshClustersVertexStorage::FLOAT_PER_VERTEX);
	m_ClusterIndexStorageBuffer.Init("VirtualGeometryIndexStorage", sizeof(uint32_t));
	m_ResourceBuffer.Init("VirtualGeometryResource", sizeof(KVirtualGeometryResource));

	return true;
}

bool KVirtualGeometryManager::UnInit()
{
	for (IKVirtualGeometryScenePtr scene : m_Scenes)
	{
		scene->UnInit();
	}
	m_Scenes.clear();

	m_ResourceBuffer.UnInit();
	m_PackedHierarchyBuffer.UnInit();
	m_ClusterBatchBuffer.UnInit();
	m_ClusterVertexStorageBuffer.UnInit();
	m_ClusterIndexStorageBuffer.UnInit();
	m_GeometryMap.clear();
	for (KVirtualGeometryResourceRef& ref : m_GeometryResources)
	{
		assert(ref.GetRefCount() == 1);
	}
	m_GeometryResources.clear();
	return true;
}

bool KVirtualGeometryManager::AcquireImpl(const char* label, const KMeshRawData& userData, KVirtualGeometryResourceRef& geometry)
{
	GeometryInfo info;
	info.path = label;

	auto it = m_GeometryMap.find(info);
	if (it == m_GeometryMap.end())
	{
		std::vector<KMeshProcessorVertex> vertices;
		std::vector<uint32_t> indices;
		if (!KMeshProcessor::ConvertForMeshProcessor(userData, vertices, indices))
		{
			return false;
		}

		KVirtualGeometryBuilder builder;
		builder.Build(vertices, indices);

		std::vector<KMeshClusterBatch> clusters;
		KMeshClustersVertexStorage vertexStroages;
		KMeshClustersIndexStorage indexStroages;
		std::vector<uint32_t> clustersPartNum;
		std::vector<uint32_t> clustersPartStart;
		if (!builder.GetMeshClusterStorages(clusters, vertexStroages, indexStroages, clustersPartNum))
		{
			return false;
		}

		clustersPartStart.resize(clustersPartNum.size());
		for (size_t i = 0; i < clustersPartNum.size(); ++i)
		{
			if (i > 0)
			{
				clustersPartStart[i] += clustersPartStart[i - 1] + clustersPartNum[i - 1];
			}
			else
			{
				clustersPartStart[i] = 0;
			}
		}

		std::vector<KMeshClusterHierarchy> hierarchies;
		if (!builder.GetMeshClusterHierarchies(hierarchies))
		{
			return false;
		}

		std::vector<KMeshClusterHierarchyPackedNode> hierarchyNode;
		hierarchyNode.resize(hierarchies.size());
		for (size_t i = 0; i < hierarchyNode.size(); ++i)
		{
			KMeshClusterHierarchy& hierarchy = hierarchies[i];
			KMeshClusterHierarchyPackedNode& node = hierarchyNode[i];
			node.lodBoundCenter = hierarchy.lodBoundCenter;
			node.lodBoundHalfExtend = hierarchy.lodBoundHalfExtend;
			for (uint32_t child = 0; child < KVirtualGeometryDefine::MAX_BVH_NODES; ++child)
			{
				node.children[child] = hierarchy.children[child];
			}
			node.lodBoundCenter = hierarchy.lodBoundCenter;

			if (hierarchy.storagePartIndex != KVirtualGeometryDefine::INVALID_INDEX)
			{
				node.isLeaf = true;
				node.clusterStart = clustersPartStart[hierarchy.storagePartIndex];
				node.clusterNum = clustersPartNum[hierarchy.storagePartIndex];
			}
			else
			{
				node.isLeaf = false;
			}
		}

		uint32_t resourceIndex = (uint32_t)m_GeometryResources.size();

		const KAABBBox& bound = builder.GetBound();

		{
			geometry = KVirtualGeometryResourceRef(KNEW KVirtualGeometryResource());

			geometry->clusterBatchOffset = (uint32_t)m_ClusterBatchBuffer.GetSize();
			geometry->clusterBatchSize = (uint32_t)clusters.size() * sizeof(KMeshClusterBatch);

			m_ClusterBatchBuffer.Append(geometry->clusterBatchSize, clusters.data());

			geometry->hierarchyPackedOffset = (uint32_t)m_PackedHierarchyBuffer.GetSize();
			geometry->hierarchyPackedSize = (uint32_t)hierarchyNode.size() * sizeof(KMeshClusterHierarchyPackedNode);

			m_PackedHierarchyBuffer.Append(geometry->hierarchyPackedSize, hierarchyNode.data());

			geometry->clusterVertexStorageOffset = (uint32_t)m_ClusterVertexStorageBuffer.GetSize();
			geometry->clusterVertexStorageSize = (uint32_t)vertexStroages.vertices.size() * sizeof(float);

			m_ClusterVertexStorageBuffer.Append(geometry->clusterVertexStorageSize, vertexStroages.vertices.data());

			geometry->clusterIndexStorageOffset = (uint32_t)m_ClusterIndexStorageBuffer.GetSize();
			geometry->clusterIndexStorageSize = (uint32_t)indexStroages.indices.size() * sizeof(uint32_t);

			m_ClusterIndexStorageBuffer.Append(geometry->clusterIndexStorageSize, indexStroages.indices.data());

			geometry->resourceIndex = resourceIndex;
			geometry->boundCenter = glm::vec4(bound.GetCenter(), 0);
			geometry->boundHalfExtend = glm::vec4(0.5f * bound.GetExtend(), 0);

			// TODO 更新材质索引
		}

		m_ResourceBuffer.Append(sizeof(KVirtualGeometryResource), geometry.Get());

		m_GeometryResources.push_back(geometry);
		m_GeometryMap[info] = geometry;

		return true;
	}
	else
	{
		geometry = it->second;
		return true;
	}
}

bool KVirtualGeometryManager::RemoveUnreferenced()
{
	std::vector<GeometryInfo> removes;
	for (auto& pair : m_GeometryMap)
	{
		if (pair.second.GetRefCount() <= 2)
		{
			removes.push_back(pair.first);
		}
	}
	for (const GeometryInfo& remove : removes)
	{
		m_GeometryMap.erase(remove);
	}

	for (uint32_t i = 0; i < (uint32_t)m_GeometryResources.size();)
	{
		if (m_GeometryResources[i].GetRefCount() <= 1)
		{
			RemoveGeometry(i);
			m_GeometryResources.erase(m_GeometryResources.begin() + i);
		}
		else
		{
			++i;
		}
	}

	for (uint32_t i = 0; i < (uint32_t)m_GeometryResources.size();)
	{
		assert(m_GeometryResources[i].GetRefCount() >= 2);
	}
	
	return true;
}

bool KVirtualGeometryManager::RemoveGeometry(uint32_t index)
{
	if (index < (uint32_t)m_GeometryResources.size())
	{
		KVirtualGeometryResourceRef geometry = m_GeometryResources[index];

		m_ResourceBuffer.Remove(index * sizeof(KVirtualGeometryResource), sizeof(KVirtualGeometryResource));

		m_PackedHierarchyBuffer.Remove(geometry->hierarchyPackedOffset, geometry->hierarchyPackedSize);
		m_ClusterBatchBuffer.Remove(geometry->clusterBatchOffset, geometry->clusterBatchSize);
		m_ClusterVertexStorageBuffer.Remove(geometry->clusterVertexStorageOffset, geometry->clusterVertexStorageSize);
		m_ClusterIndexStorageBuffer.Remove(geometry->clusterIndexStorageOffset, geometry->clusterIndexStorageSize);

		// TODO 更新材质索引

		for (uint32_t i = index + 1; i < (uint32_t)m_GeometryResources.size(); ++i)
		{
			--m_GeometryResources[i]->resourceIndex;
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool KVirtualGeometryManager::Update()
{
	RemoveUnreferenced();
	return true;
}

bool KVirtualGeometryManager::ReloadShader()
{
	for (IKVirtualGeometryScenePtr scene : m_Scenes)
	{
		KVirtualGeometryScene* vgScene = (KVirtualGeometryScene*)scene.get();
		vgScene->ReloadShader();
	}
	return true;
}

bool KVirtualGeometryManager::AcquireFromUserData(const KMeshRawData& userData, const std::string& label, KVirtualGeometryResourceRef& ref)
{
	return AcquireImpl(label.c_str(), userData, ref);
}

bool KVirtualGeometryManager::CreateVirtualGeometryScene(IKVirtualGeometryScenePtr& scene)
{
	scene = IKVirtualGeometryScenePtr(KNEW KVirtualGeometryScene());
	m_Scenes.insert(scene);
	return true;
}

bool KVirtualGeometryManager::RemoveVirtualGeometryScene(IKVirtualGeometryScenePtr& scene)
{
	auto it = m_Scenes.find(scene);
	if (it != m_Scenes.end())
		m_Scenes.erase(it);
	return true;
}

bool KVirtualGeometryManager::Execute(IKCommandBufferPtr primaryBuffer)
{
	for (IKVirtualGeometryScenePtr scene : m_Scenes)
	{
		scene->Execute(primaryBuffer);
	}
	return true;
}