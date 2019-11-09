#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/Asset/KMesh.h"

class KMeshManager
{
protected:
	struct MeshUsingInfo
	{
		size_t useCount;
		KMeshPtr mesh;
	};
	typedef std::map<std::string, MeshUsingInfo> MeshMap;
	MeshMap m_Meshes;
	IKRenderDevice* m_Device;
	size_t m_FrameInFlight;
public:
	KMeshManager();
	~KMeshManager();

	bool Init(IKRenderDevice* device, size_t frameInFlight);
	bool UnInit();

	bool Acquire(const char* path, KMeshPtr& ptr);
	bool Release(KMeshPtr& ptr);
};