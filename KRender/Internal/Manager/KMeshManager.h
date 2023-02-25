#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/Asset/KMesh.h"
#include <map>

class KMeshManager
{
protected:
	struct MeshInfo
	{
		std::string path;
		bool hostVisible = false;

		bool operator<(const MeshInfo& rhs) const
		{
			if (path != rhs.path)
				return path < rhs.path;
			return hostVisible < rhs.hostVisible;
		}
	};
	typedef std::map<MeshInfo, KMeshRef> MeshMap;

	MeshMap m_Meshes;
	IKRenderDevice* m_Device;

	bool AcquireImpl(const char* path, bool fromAsset, bool hostVisible, KMeshRef& ref);
public:
	KMeshManager();
	~KMeshManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Acquire(const char* path, KMeshRef& ref, bool hostVisible = false);
	bool AcquireFromAsset(const char* path, KMeshRef& ref, bool hostVisible = false);
	bool AcquireAsUtility(const KMeshUtilityInfoPtr& info, KMeshRef& ref);

	bool UpdateUtility(const KMeshUtilityInfoPtr& info, KMeshRef& ref);

	bool AcquireOCQuery(std::vector<IKQueryPtr>& queries);
	bool ReleaseOCQuery(std::vector<IKQueryPtr>& queries);
};