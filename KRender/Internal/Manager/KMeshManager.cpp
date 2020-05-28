#include "KMeshManager.h"
#include "Internal/Asset/Utility/KMeshUtilityImpl.h"
#include "Interface/IKQuery.h"

KMeshManager::KMeshManager()
	: m_Device(nullptr),
	m_FrameInFlight(0)
{
}

KMeshManager::~KMeshManager()
{
	assert(m_Meshes.empty());
	assert(m_SpecialMesh.empty());
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

	for (KMeshPtr mesh : m_SpecialMesh)
	{
		mesh->UnInit();
	}
	m_SpecialMesh.clear();

	m_Device = nullptr;
	m_FrameInFlight = 0;

	return true;
}

bool KMeshManager::AcquireImpl(const char* path, bool fromAsset, bool hostVisible, KMeshPtr& ptr)
{
	auto it = m_Meshes.find(path);

	if(it != m_Meshes.end())
	{
		MeshUsingInfo& info = it->second;
		info.useCount += 1;
		ptr = info.mesh;
		return true;
	}

	ptr = KMeshPtr(KNEW KMesh());

	bool bRetValue = false;
	if(fromAsset)
	{
		bRetValue = ptr->InitFromAsset(path, m_Device, m_FrameInFlight, hostVisible);
	}
	else
	{
		bRetValue = ptr->InitFromFile(path, m_Device, m_FrameInFlight, hostVisible);
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

bool KMeshManager::Acquire(const char* path, KMeshPtr& ptr, bool hostVisible)
{
	return AcquireImpl(path, false, hostVisible, ptr);
}

bool KMeshManager::AcquireFromAsset(const char* path, KMeshPtr& ptr, bool hostVisible)
{
	return AcquireImpl(path, true, hostVisible, ptr);
}

bool KMeshManager::Release(KMeshPtr& ptr)
{
	if(ptr)
	{
		const auto& path = ptr->GetPath();

		if (!path.empty())
		{
			auto it = m_Meshes.find(path);
			if (it != m_Meshes.end())
			{
				MeshUsingInfo& info = it->second;
				info.useCount -= 1;

				if (info.useCount == 0)
				{
					// 等待设备空闲
					m_Device->Wait();

					ptr->UnInit();
					m_Meshes.erase(it);
				}

				ptr = nullptr;
				return true;
			}
		}
		else
		{
			auto it = m_SpecialMesh.find(ptr);
			if (it != m_SpecialMesh.end())
			{
				m_SpecialMesh.erase(it);

				// 等待设备空闲
				m_Device->Wait();

				ptr->UnInit();
				ptr = nullptr;
				return true;
			}
		}
	}
	return false;
}

bool KMeshManager::AcquireAsUtility(const KMeshUtilityInfoPtr& info, KMeshPtr& ptr)
{
	ptr = KMeshPtr(KNEW KMesh());
	if (ptr->InitUtility(info, m_Device, m_FrameInFlight))
	{
		m_SpecialMesh.insert(ptr);
		return true;
	}
	ptr = nullptr;
	return false;
}

bool KMeshManager::UpdateUtility(const KMeshUtilityInfoPtr& info, KMeshPtr& ptr)
{
	if (ptr)
	{
		m_Device->Wait();
		return ptr->UpdateUnility(info, m_Device, m_FrameInFlight);
	}
	return false;
}

bool KMeshManager::AcquireOCQuery(std::vector<IKQueryPtr>& queries)
{
	ReleaseOCQuery(queries);
	queries.resize(m_FrameInFlight);
	for (IKQueryPtr& query : queries)
	{
		m_Device->CreateQuery(query);
		query->Init(QT_OCCLUSION);
	}
	return true;
}

bool KMeshManager::ReleaseOCQuery(std::vector<IKQueryPtr>& queries)
{
	for (IKQueryPtr& query : queries)
	{
		query->UnInit();
		query = nullptr;
	}
	queries.clear();
	return true;
}