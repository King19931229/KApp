#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/Asset/KMesh.h"
#include <unordered_map>

class KMeshManager
{
protected:
	struct MeshUsingInfo
	{
		size_t useCount;
		KMeshPtr mesh;
	};
	typedef std::unordered_map<std::string, MeshUsingInfo> MeshMap;
	MeshMap m_Meshes;
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
};