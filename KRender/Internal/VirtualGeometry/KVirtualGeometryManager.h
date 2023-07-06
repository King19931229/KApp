#pragma once
#include "KVirtualGeomerty.h"
#include "Interface/IKBuffer.h"
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
	bool Append(size_t size, void* pData);

	size_t GetSize() const { return m_Size; }
	IKStorageBufferPtr GetBuffer() { return m_Buffer; }
};

class KVirtualGeometryManager
{
protected:
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

	KVirtualGeometryStorageBuffer m_PackedHierarchyBuffer;
	KVirtualGeometryStorageBuffer m_ClusterBatchBuffer;
	KVirtualGeometryStorageBuffer m_ClusterStorageBuffer;

	bool AcquireImpl(const char* label, const KMeshRawData& userData, KVirtualGeometryResourceRef& geometry);
	bool RemoveUnreferenced();
	bool RemoveGeometry(uint32_t index);
public:
	KVirtualGeometryManager();
	~KVirtualGeometryManager();

	bool Init();
	bool UnInit();

	bool Update();

	IKStorageBufferPtr GetPackedHierarchyBuffer() { return m_PackedHierarchyBuffer.GetBuffer(); }
	IKStorageBufferPtr GetClusterBatchBuffer() { return m_ClusterBatchBuffer.GetBuffer(); }
	IKStorageBufferPtr GetClusterStorageBuffer() { return m_ClusterStorageBuffer.GetBuffer(); }

	bool AcquireFromUserData(const KMeshRawData& userData, const std::string& label, KVirtualGeometryResourceRef& ref);
};