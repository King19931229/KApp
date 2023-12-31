#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKBuffer.h"
#include "KVirtualGeomerty.h"
#include "KVirtualGeometryStreaming.h"
#include <map>

class KVirtualGeometryStorageBuffer
{
protected:
	std::string m_Name;
	size_t m_Size;
	IKStorageBufferPtr m_Buffer;
public:
	KVirtualGeometryStorageBuffer();
	~KVirtualGeometryStorageBuffer();

	bool Init(const char* name, size_t initialSize);
	bool UnInit();
	bool Remove(size_t offset, size_t size);
	bool Modify(size_t offset, size_t size, void* pData);
	bool Append(size_t size, void* pData);

	size_t GetSize() const { return m_Size; }
	IKStorageBufferPtr GetBuffer() { return m_Buffer; }
};

class KVirtualGeometryManager
{
protected:
	std::unordered_set<IKVirtualGeometryScenePtr> m_Scenes;

	struct GeometryInfo
	{
		std::string path;
		bool operator<(const GeometryInfo& rhs) const
		{
			return path < rhs.path;
		}
	};
	typedef std::map<GeometryInfo, KVirtualGeometryResourceRef> GeometryMap;

	GeometryMap m_GeometryMap;
	std::vector<KVirtualGeometryResourceRef> m_GeometryResources;
	std::vector<KMaterialRef> m_MaterialResources;

	KVirtualGeometryStorageBuffer m_PackedHierarchyBuffer;
	KVirtualGeometryStorageBuffer m_ClusterBatchBuffer;
	KVirtualGeometryStorageBuffer m_ClusterVertexStorageBuffer;
	KVirtualGeometryStorageBuffer m_ClusterIndexStorageBuffer;
	KVirtualGeometryStorageBuffer m_ClusterMateialStorageBuffer;
	KVirtualGeometryStorageBuffer m_ResourceBuffer;

	KShaderCompileEnvironment m_DefaultBindingEnv;
	KShaderCompileEnvironment m_BasepassBindingEnv;

	KVirtualGeometryStreamingManager m_StreamingManager;

	bool m_UseMeshPipeline;
	bool m_UseDoubleOcclusion;
	bool m_PersistentCull;

	bool AcquireImpl(const char* label, const KMeshRawData& userData, KVirtualGeometryResourceRef& geometry);
	bool RemoveGeometry(uint32_t index);
public:
	KVirtualGeometryManager();
	~KVirtualGeometryManager();

	bool Init();
	bool UnInit();

	bool Update();
	bool ReloadShader();

	bool RemoveUnreferenced();

	IKStorageBufferPtr GetPackedHierarchyBuffer() { return m_PackedHierarchyBuffer.GetBuffer(); }
	IKStorageBufferPtr GetClusterBatchBuffer() { return m_ClusterBatchBuffer.GetBuffer(); }
	IKStorageBufferPtr GetClusterVertexStorageBuffer() { return m_ClusterVertexStorageBuffer.GetBuffer(); }
	IKStorageBufferPtr GetClusterIndexStorageBuffer() { return m_ClusterIndexStorageBuffer.GetBuffer(); }
	IKStorageBufferPtr GetClusterMaterialStorageBuffer() { return m_ClusterMateialStorageBuffer.GetBuffer(); }
	IKStorageBufferPtr GetResourceBuffer() { return m_ResourceBuffer.GetBuffer(); }

	bool& GetUseMeshPipeline() { return m_UseMeshPipeline; }
	bool& GetUseDoubleOcclusion() { return m_UseDoubleOcclusion; }
	bool& GetUsePersistentCull() { return m_PersistentCull; }

	const KShaderCompileEnvironment& GetDefaultBindingEnv() const { return m_DefaultBindingEnv; }
	const KShaderCompileEnvironment& GetBasepassBindingEnv() const { return m_BasepassBindingEnv; }

	const std::vector<KMaterialRef>& GetAllMaterials() const { return m_MaterialResources; }

	bool AcquireFromUserData(const KMeshRawData& userData, const std::string& label, KVirtualGeometryResourceRef& ref);

	bool CreateVirtualGeometryScene(IKVirtualGeometryScenePtr& scene);
	bool RemoveVirtualGeometryScene(IKVirtualGeometryScenePtr& scene);

	bool ExecuteMain(IKCommandBufferPtr primaryBuffer);
	bool ExecutePost(IKCommandBufferPtr primaryBuffer);

	IKStorageBufferPtr GetStreamingRequestPipeline(uint32_t frameIndex);
	IKStorageBufferPtr GetPageDataBuffer();
	IKUniformBufferPtr GetStreamingDataBuffer();

	KVirtualGeometryResourceRef GetResource(uint32_t resourceIndex);
};