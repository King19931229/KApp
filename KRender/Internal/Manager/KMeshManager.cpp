#include "KMeshManager.h"

KMeshManager::KMeshManager()
	: m_Device(nullptr),
	m_FrameInFlight(0)
{
}

KMeshManager::~KMeshManager()
{
	assert(m_Meshes.empty());
}

bool KMeshManager::Init(IKRenderDevice* device, size_t frameInFlight)
{
	UnInit();

	m_Device = device;
	m_FrameInFlight = frameInFlight;
	return true;
}

bool KMeshManager::UnInit()
{
	for(auto it = m_Meshes.begin(), itEnd = m_Meshes.end(); it != itEnd; ++it)
	{
		MeshUsingInfo& info = it->second;
		assert(info.mesh);
		info.mesh->UnInit();
	}
	m_Meshes.clear();

	m_Device = nullptr;
	m_FrameInFlight = 0;

	return true;
}

bool KMeshManager::AcquireImpl(const char* path, bool fromAsset, KMeshPtr& ptr)
{
	auto it = m_Meshes.find(path);

	if(it != m_Meshes.end())
	{
		MeshUsingInfo& info = it->second;
		info.useCount += 1;
		ptr = info.mesh;
		return true;
	}

	ptr = KMeshPtr(new KMesh());

	bool bRetValue = false;
	if(fromAsset)
	{
		bRetValue = ptr->InitFromAsset(path, m_Device, m_FrameInFlight);
	}
	else
	{
		bRetValue = ptr->InitFromFile(path, m_Device, m_FrameInFlight);
	}

	if(bRetValue)
	{
		MeshUsingInfo info = { 1, ptr };
		m_Meshes[path] = info;
		return true;
	}

	ptr = nullptr;
	return false;
}

bool KMeshManager::Acquire(const char* path, KMeshPtr& ptr)
{
	return AcquireImpl(path, false, ptr);
}

bool KMeshManager::AcquireFromAsset(const char* path, KMeshPtr& ptr)
{
	return AcquireImpl(path, true, ptr);
}

bool KMeshManager::Release(KMeshPtr& ptr)
{
	if(ptr)
	{
		auto it = m_Meshes.find(ptr->GetPath());
		if(it != m_Meshes.end())
		{
			MeshUsingInfo& info = it->second;
			info.useCount -= 1;

			if(info.useCount == 0)
			{
				ptr->UnInit();
				m_Meshes.erase(it);
			}

			ptr = nullptr;
			return true;
		}
	}
	return false;
}