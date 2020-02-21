#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/Asset/KMesh.h"
#include <unordered_map>
#include <unordered_set>

class KMeshManager
{
protected:
	struct MeshUsingInfo
	{
		size_t useCount;
		KMeshPtr mesh;
	};

	typedef std::unordered_map<std::string, MeshUsingInfo> MeshMap;
	typedef std::unordered_set<KMeshPtr> SpecialMesh;

	MeshMap m_Meshes;
	SpecialMesh m_SpecialMesh;

	IKRenderDevice* m_Device;
	size_t m_FrameInFlight;

	bool AcquireImpl(const char* path, bool fromAsset, KMeshPtr& ptr);
public:
	KMeshManager();
	~KMeshManager();

	bool Init(IKRenderDevice* device, size_t frameInFlight);
	bool UnInit();

	bool Acquire(const char* path, KMeshPtr& ptr);
	bool AcquireFromAsset(const char* path, KMeshPtr& ptr);
	bool Release(KMeshPtr& ptr);

	// utility
	bool CreateBox(const KAABBBox& box, KMeshPtr& ptr);
};