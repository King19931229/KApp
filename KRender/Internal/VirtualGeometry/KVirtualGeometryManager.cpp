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
	m_Buffer->InitDevice(false, false);
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
		void* pMapped = nullptr;

		std::vector<unsigned char> bufferData;

		bufferData.resize(m_Size);
		m_Buffer->Map(&pMapped);
		memcpy(bufferData.data(), pMapped, m_Size);
		m_Buffer->UnMap();

		size_t newBufferSize = std::max((size_t)1, KMath::SmallestPowerOf2GreaterEqualThan(m_Size - size));

		if (newBufferSize < m_Buffer->GetBufferSize())
		{
			m_Buffer->UnInit();
			m_Buffer->InitMemory(newBufferSize, nullptr);
			m_Buffer->InitDevice(false, false);
			m_Buffer->SetDebugName(m_Name.c_str());
		}

		m_Buffer->Map(&pMapped);
		memcpy(POINTER_OFFSET(pMapped, 0), bufferData.data(), offset);
		memcpy(POINTER_OFFSET(pMapped, offset), bufferData.data() + offset + size, m_Size - size - offset);
		m_Buffer->UnMap();

		m_Size -= size;

		return true;
	}
	else
	{
		return false;
	}
}

bool KVirtualGeometryStorageBuffer::Modify(size_t offset, size_t size, void* pData)
{
	if (m_Buffer == nullptr)
	{
		return false;
	}

	if (offset + size <= m_Size)
	{
		void* pMapped = nullptr;
		m_Buffer->Map(&pMapped);
		memcpy(POINTER_OFFSET(pMapped, offset), pData, size);
		m_Buffer->UnMap();
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

	void* pMapped = nullptr;

	if (newBufferSize > m_Buffer->GetBufferSize())
	{
		KRenderGlobal::RenderDevice->Wait();

		std::vector<unsigned char> bufferData;
		if (m_Size > 0)
		{
			bufferData.resize(m_Size);
			m_Buffer->Map(&pMapped);
			memcpy(bufferData.data(), pMapped, m_Size);
			m_Buffer->UnMap();
		}

		m_Buffer->UnInit();
		m_Buffer->InitMemory(newBufferSize, nullptr);
		m_Buffer->InitDevice(false, false);
		m_Buffer->SetDebugName(m_Name.c_str());

		if (m_Size > 0)
		{
			m_Buffer->Map(&pMapped);
			memcpy(POINTER_OFFSET(pMapped, 0), bufferData.data(), m_Size);
			m_Buffer->UnMap();
		}
	}

	m_Buffer->Map(&pMapped);
	memcpy(POINTER_OFFSET(pMapped, m_Size), pData, size);
	m_Buffer->UnMap();

	m_Size += size;

	return true;
}

KVirtualGeometryManager::KVirtualGeometryManager()
	: m_UseMeshPipeline(false)
	, m_UseDoubleOcclusion(true)
	, m_PersistentCull(true)
	, m_ConeCull(true)
{
#define VIRTUAL_GEOMETRY_BINDING_TO_STR(x) #x

#define VIRTUAL_GEOMETRY_BINDING(SEMANTIC) m_DefaultBindingEnv.macros.push_back( {VIRTUAL_GEOMETRY_BINDING_TO_STR(BINDING_##SEMANTIC), std::to_string(BINDING_##SEMANTIC) });
#include "KVirtualGeomertyBinding.inl"
#undef VIRTUAL_GEOMETRY_BINDING

#define VIRTUAL_GEOMETRY_BINDING(SEMANTIC) m_BasepassBindingEnv.macros.push_back( {VIRTUAL_GEOMETRY_BINDING_TO_STR(BINDING_##SEMANTIC), std::to_string(MAX_USER_TEXTURE_BINDING + BINDING_##SEMANTIC) });
#include "KVirtualGeomertyBinding.inl"
#undef VIRTUAL_GEOMETRY_BINDING

#undef VIRTUAL_GEOMETRY_BINDING_TO_STR
}

KVirtualGeometryManager::~KVirtualGeometryManager()
{
	assert(m_GeometryMap.empty());
}

bool KVirtualGeometryManager::Init()
{
	UnInit();

	m_ResourceBuffer.Init("VirtualGeometryResource", sizeof(KVirtualGeometryResource));

	m_PackedHierarchyBuffer.Init("VirtualGeometryPackedHierarchy", sizeof(glm::uvec4));
	m_ClusterBatchBuffer.Init("VirtualGeometryClusterBatch", sizeof(glm::uvec4));
	m_ClusterVertexStorageBuffer.Init("VirtualGeometryVertexStorage", sizeof(float) * KVirtualGeometryEncoding::FLOAT_PER_VERTEX);
	m_ClusterIndexStorageBuffer.Init("VirtualGeometryIndexStorage", sizeof(uint32_t));
	m_ClusterMateialStorageBuffer.Init("VirtualGeometryMaterialStorage", sizeof(uint32_t) * KVirtualGeometryEncoding::INT_PER_MATERIAL);

	m_StreamingManager.Init(500, 5);

	return true;
}

bool KVirtualGeometryManager::UnInit()
{
	m_StreamingManager.UnInit();

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
	m_ClusterMateialStorageBuffer.UnInit();
	m_GeometryMap.clear();

	for (KVirtualGeometryResourceRef& ref : m_GeometryResources)
	{
		assert(ref.GetRefCount() == 1);
	}
	m_GeometryResources.clear();

	for (KMaterialRef& ref : m_MaterialResources)
	{
		assert(ref.GetRefCount() == 1);
	}
	m_MaterialResources.clear();

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
		std::vector<uint32_t> materialIndices;
		if (!KMeshProcessor::ConvertForMeshProcessor(userData, vertices, indices, materialIndices))
		{
			return false;
		}

		KVirtualGeometryBuilder builder;
		builder.Build(vertices, indices, materialIndices);

		KVirtualGeometryPages pages;
		KVirtualGeometryPageStorages pageStorages;
		KVirtualGeomertyFixup pageFixup;
		KVirtualGeomertyPageDependencies pageDependencies;
		KVirtualGeomertyPageClustersData pageClusters;
		if (!builder.GetPages(pages, pageStorages, pageFixup, pageDependencies, pageClusters))
		{
			return false;
		}

		KMeshClusterBatchStorage batchStorages;
		KMeshClustersVertexStorage vertexStroages;
		KMeshClustersIndexStorage indexStroages;
		KMeshClustersMaterialStorage materialStorages;

		if (!builder.GetMeshClusterStorages(batchStorages, vertexStroages, indexStroages, materialStorages))
		{
			return false;
		}

		std::vector<KMeshClusterHierarchy> hierarchies;
		if (!builder.GetMeshClusterHierarchies(hierarchies))
		{
			return false;
		}

		std::vector<KMeshClusterPtr> clusters;
		std::vector<KMeshClusterGroupPartPtr> clusterGroupParts;
		std::vector<KMeshClusterGroupPtr> clusterGroups;
		if (!builder.GetMeshClusterGroupParts(clusters, clusterGroupParts, clusterGroups))
		{
			return false;
		}

		std::vector<KMeshClusterHierarchyPackedNode> hierarchyNode;
		hierarchyNode.resize(hierarchies.size());

		std::vector<uint32_t> clusterNumOfPreviousPages;
		clusterNumOfPreviousPages.resize(pages.pages.size());
		memset(clusterNumOfPreviousPages.data(), 0, clusterNumOfPreviousPages.size() * sizeof(uint32_t));
		for (size_t i = 1; i < clusterNumOfPreviousPages.size(); ++i)
		{
			clusterNumOfPreviousPages[i] = clusterNumOfPreviousPages[i - 1] + pages.pages[i - 1].clusterNum;
		}

		for (size_t i = 0; i < hierarchyNode.size(); ++i)
		{
			KMeshClusterHierarchy& hierarchy = hierarchies[i];
			KMeshClusterHierarchyPackedNode& node = hierarchyNode[i];

			node.lodBoundCenterError = hierarchy.lodBoundCenterError;
			node.lodBoundHalfExtendRadius = hierarchy.lodBoundHalfExtendRadius;

			for (uint32_t child = 0; child < KVirtualGeometryDefine::MAX_BVH_NODES; ++child)
			{
				node.children[child] = hierarchy.children[child];
			}

			if (hierarchy.partIndex != KVirtualGeometryDefine::INVALID_INDEX)
			{
				KMeshClusterGroupPartPtr part = clusterGroupParts[hierarchy.partIndex];
				uint32_t pageIndex = part->pageIndex;
				uint32_t groupIndex = part->groupIndex;
				KMeshClusterGroupPtr group = clusterGroups[groupIndex];
				node.gpuPageIndex = KVirtualGeometryDefine::INVALID_INDEX;
				node.clusterPageStart = part->clusterStart;
				node.clusterStart = node.clusterPageStart + clusterNumOfPreviousPages[pageIndex];
				node.clusterNum = (uint32_t)part->clusters.size();
				node.groupPageStart = group->pageStart;
				node.groupPageNum = group->pageEnd - group->pageStart + 1;
				node.isLeaf = true;
			}
			else
			{
				node.isLeaf = false;
			}
		}

		uint32_t resourceIndex = (uint32_t)m_GeometryResources.size();
		uint32_t resourceIndexInStreaming = m_StreamingManager.AddGeometry(pages, pageStorages, pageFixup, pageDependencies, pageClusters);
		assert(resourceIndex == resourceIndexInStreaming);

		const KAABBBox& bound = builder.GetBound();

		{
			geometry = KVirtualGeometryResourceRef(KNEW KVirtualGeometryResource());

			geometry->clusterBatchOffset = (uint32_t)m_ClusterBatchBuffer.GetSize();
			geometry->clusterBatchSize = (uint32_t)batchStorages.batches.size() * sizeof(KMeshClusterBatch);

			geometry->hierarchyPackedOffset = (uint32_t)m_PackedHierarchyBuffer.GetSize();
			geometry->hierarchyPackedSize = (uint32_t)hierarchyNode.size() * sizeof(KMeshClusterHierarchyPackedNode);

			geometry->clusterVertexStorageByteOffset = (uint32_t)m_ClusterVertexStorageBuffer.GetSize();
			geometry->clusterVertexStorageByteSize = (uint32_t)vertexStroages.vertices.size() * sizeof(float);

			geometry->clusterIndexStorageByteOffset = (uint32_t)m_ClusterIndexStorageBuffer.GetSize();
			geometry->clusterIndexStorageByteSize = (uint32_t)indexStroages.indices.size() * sizeof(uint32_t);

			geometry->resourceIndex = resourceIndex;
			geometry->boundCenter = glm::vec4(bound.GetCenter(), 0);
			geometry->boundHalfExtend = glm::vec4(0.5f * bound.GetExtend(), 0);

			geometry->materialBaseIndex = (uint32_t)m_MaterialResources.size();
			geometry->materialNum = (uint32_t)userData.parts.size();

			geometry->clusterMaterialStorageByteOffset = (uint32_t)m_ClusterMateialStorageBuffer.GetSize();
			geometry->clusterMaterialStorageByteSize = (uint32_t)materialStorages.materials.size() * sizeof(uint32_t);

			m_ClusterBatchBuffer.Append(geometry->clusterBatchSize, batchStorages.batches.data());
			m_PackedHierarchyBuffer.Append(geometry->hierarchyPackedSize, hierarchyNode.data());
			m_ClusterVertexStorageBuffer.Append(geometry->clusterVertexStorageByteSize, vertexStroages.vertices.data());
			m_ClusterIndexStorageBuffer.Append(geometry->clusterIndexStorageByteSize, indexStroages.indices.data());
			m_ClusterMateialStorageBuffer.Append(geometry->clusterMaterialStorageByteSize, materialStorages.materials.data());
		}

		for (uint32_t i = 0; i < userData.parts.size(); ++i)
		{
			KMaterialRef material;
			if (!KRenderGlobal::MaterialManager.Create(userData.parts[i].material, material, false))
			{
				KRenderGlobal::MaterialManager.GetMissingMaterial(material);
			}
			m_MaterialResources.push_back(material);
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

	for (uint32_t i = 0; i < (uint32_t)m_GeometryResources.size(); ++i)
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

		m_StreamingManager.RemoveGeometry(index);

		m_ResourceBuffer.Remove(index * sizeof(KVirtualGeometryResource), sizeof(KVirtualGeometryResource));

		m_PackedHierarchyBuffer.Remove(geometry->hierarchyPackedOffset, geometry->hierarchyPackedSize);
		m_ClusterBatchBuffer.Remove(geometry->clusterBatchOffset, geometry->clusterBatchSize);
		m_ClusterVertexStorageBuffer.Remove(geometry->clusterVertexStorageByteOffset, geometry->clusterVertexStorageByteSize);
		m_ClusterIndexStorageBuffer.Remove(geometry->clusterIndexStorageByteOffset, geometry->clusterIndexStorageByteSize);
		m_ClusterMateialStorageBuffer.Remove(geometry->clusterMaterialStorageByteOffset, geometry->clusterMaterialStorageByteSize);

		uint32_t materialBaseIndex = geometry->materialBaseIndex;
		uint32_t materialNum = geometry->materialNum;
		m_MaterialResources.erase(m_MaterialResources.begin() + materialBaseIndex, m_MaterialResources.begin() + materialBaseIndex + materialNum);

		std::vector<KVirtualGeometryResource> resources;
		resources.reserve(m_GeometryResources.size() - 1);

		for (uint32_t i = index + 1; i < (uint32_t)m_GeometryResources.size(); ++i)
		{
			m_GeometryResources[i]->resourceIndex -= 1;
			m_GeometryResources[i]->materialBaseIndex -= materialNum;
			m_GeometryResources[i]->hierarchyPackedOffset -= geometry->hierarchyPackedSize;
			m_GeometryResources[i]->clusterBatchOffset -= geometry->clusterBatchSize;
			m_GeometryResources[i]->clusterVertexStorageByteOffset -= geometry->clusterVertexStorageByteSize;
			m_GeometryResources[i]->clusterIndexStorageByteOffset -= geometry->clusterIndexStorageByteSize;
			m_GeometryResources[i]->clusterMaterialStorageByteOffset -= geometry->clusterMaterialStorageByteSize;

			resources.push_back(*m_GeometryResources[i].Get());
		}

		m_ResourceBuffer.Modify(index * sizeof(KVirtualGeometryResource), resources.size() * sizeof(KVirtualGeometryResource), resources.data());

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
	m_StreamingManager.ReloadShader();
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

bool KVirtualGeometryManager::ExecuteMain(IKCommandBufferPtr primaryBuffer)
{
	m_StreamingManager.Update(primaryBuffer);
	for (IKVirtualGeometryScenePtr scene : m_Scenes)
	{
		scene->ExecuteMain(primaryBuffer);
	}
	return true;
}

bool KVirtualGeometryManager::ExecutePost(IKCommandBufferPtr primaryBuffer)
{
	for (IKVirtualGeometryScenePtr scene : m_Scenes)
	{
		scene->ExecutePost(primaryBuffer);
	}
	return true;
}

IKStorageBufferPtr KVirtualGeometryManager::GetStreamingRequestPipeline(uint32_t frameIndex)
{
	return m_StreamingManager.GetStreamingRequestPipeline(frameIndex);
}

IKStorageBufferPtr KVirtualGeometryManager::GetPageDataBuffer()
{
	return m_StreamingManager.GetPageDataBuffer();
}

IKUniformBufferPtr KVirtualGeometryManager::GetStreamingDataBuffer()
{
	return m_StreamingManager.GetStreamingDataBuffer();
}

KVirtualGeometryResourceRef KVirtualGeometryManager::GetResource(uint32_t resourceIndex)
{
	if (resourceIndex < m_GeometryResources.size())
	{
		return m_GeometryResources[resourceIndex];
	}
	else
	{
		return KVirtualGeometryResourceRef();
	}
}