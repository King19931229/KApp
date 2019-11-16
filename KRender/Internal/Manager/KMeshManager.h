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
	size_t m_RenderThreadNum;

	bool AcquireImpl(const char* path, bool fromAsset, KMeshPtr& ptr);
public:
	KMeshManager();
	~KMeshManager();

	bool Init(IKRenderDevice* device, size_t frameInFlight, size_t renderThreadNum);
	bool UnInit();

	bool Acquire(const char* path, KMeshPtr& ptr);
	bool AcquireFromAsset(const char* path, KMeshPtr& ptr);
	bool Release(KMeshPtr& ptr);
};