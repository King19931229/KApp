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

		bool operator<(const MeshInfo& rhs) const
		{
			return path < rhs.path;	
		}
	};
	typedef std::map<MeshInfo, KMeshRef> MeshMap;

	MeshMap m_Meshes;
	bool AcquireImpl(const char* path, bool fromAsset, KMeshRef& ref);
public:
	KMeshManager();
	~KMeshManager();

	bool Init();
	bool UnInit();

	bool Acquire(const char* path, KMeshRef& ref);
	bool AcquireFromAsset(const char* path, KMeshRef& ref);
	bool AcquireFromUserData(const KMeshRawData& userData, const std::string& label, KMeshRef& ref);
	bool New(KMeshRef& ref);

	bool AcquireOCQuery(std::vector<IKQueryPtr>& queries);
	bool ReleaseOCQuery(std::vector<IKQueryPtr>& queries);
};