#pragma once
#include "KVirtualGeomerty.h"
#include "Interface/IKBuffer.h"
#include <map>

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

	IKStorageBufferPtr m_PackedHierarchyBuffer;
	IKStorageBufferPtr m_PackedClusterBuffer;
	IKStorageBufferPtr m_ClusterStorageBuffer;

	bool AcquireImpl(const char* label, const KMeshRawData& userData, KVirtualGeometryResourceRef& geometry);
	bool RemoveUnreferenced();

	bool RemoveGeometry(uint32_t index);
	bool AppendGeometry(KVirtualGeometryResourceRef geometry, const std::vector<KMeshClustersStorage>& stroages, const std::vector<KMeshClusterHierarchy>& hierarchies);
public:
	KVirtualGeometryManager();
	~KVirtualGeometryManager();

	bool Init();
	bool UnInit();

	bool Update();

	IKStorageBufferPtr GetPackedHierarchyBuffer() { return m_PackedHierarchyBuffer; }
	IKStorageBufferPtr GetPackedClusterBuffer() { return m_PackedClusterBuffer; }

	bool AcquireFromUserData(const KMeshRawData& userData, const std::string& label, KVirtualGeometryResourceRef& ref);
};