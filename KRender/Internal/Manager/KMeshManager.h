#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/Asset/KMesh.h"
#include <unordered_map>
#include <unordered_set>

class KMeshManager
{
protected:
	typedef std::unordered_map<std::string, KMeshRef> MeshMap;

	MeshMap m_Meshes;
	IKRenderDevice* m_Device;

	bool AcquireImpl(const char* path, bool fromAsset, bool hostVisible, KMeshRef& ref);
	bool Release(KMeshPtr& ref);
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