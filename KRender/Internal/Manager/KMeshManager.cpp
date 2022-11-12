#include "KMeshManager.h"
#include "Internal/Asset/Utility/KMeshUtilityImpl.h"
#include "Internal/KRenderGlobal.h"
#include "Interface/IKQuery.h"

KMeshManager::KMeshManager()
	: m_Device(nullptr)
{
}

KMeshManager::~KMeshManager()
{
	assert(m_Meshes.empty());
}

bool KMeshManager::Init(IKRenderDevice* device)
{
	UnInit();
	m_Device = device;
	return true;
}

bool KMeshManager::UnInit()
{
	for(auto it = m_Meshes.begin(), itEnd = m_Meshes.end(); it != itEnd; ++it)
	{
		KMeshRef& ref = it->second;
		ASSERT_RESULT(ref.GetRefCount() == 1);
	}
	m_Meshes.clear();
	m_Device = nullptr;
	return true;
}

bool KMeshManager::AcquireImpl(const char* path, bool fromAsset, bool hostVisible, KMeshRef& ref)
{
	auto it = m_Meshes.find(path);

	if(it != m_Meshes.end())
	{
		ref = it->second;
		return true;
	}

	KMeshPtr ptr = KMeshPtr(KNEW KMesh());
	bool bRetValue = false;
	if(fromAsset)
	{
		bRetValue = ptr->InitFromAsset(path, m_Device, hostVisible);
	}
	else
	{
		bRetValue = ptr->InitFromFile(path, m_Device, hostVisible);
	}

	if(bRetValue)
	{
		ref = KMeshRef(ptr, [this](KMeshPtr ptr)
		{
			Release(ptr);
		});
		m_Meshes[path] = ref;
		return true;
	}

	return false;
}

bool KMeshManager::Acquire(const char* path, KMeshRef& ref, bool hostVisible)
{
	return AcquireImpl(path, false, hostVisible, ref);
}

bool KMeshManager::AcquireFromAsset(const char* path, KMeshRef& ref, bool hostVisible)
{
	return AcquireImpl(path, true, hostVisible, ref);
}

bool KMeshManager::Release(KMeshPtr& ptr)
{
	if(ptr)
	{
		m_Device->Wait();
		ptr->UnInit();
		const auto& path = ptr->GetPath();
		if (!path.empty())
		{
			auto it = m_Meshes.find(path);
			if (it != m_Meshes.end())
			{				
				m_Meshes.erase(it);
				ptr = nullptr;
			}
		}
		return true;
	}
	return false;
}

bool KMeshManager::AcquireAsUtility(const KMeshUtilityInfoPtr& info, KMeshRef& ref)
{
	KMeshPtr ptr = KMeshPtr(KNEW KMesh());
	if (ptr->InitUtility(info, m_Device))
	{
		ref = KMeshRef(ptr, [this](KMeshPtr ptr)
		{
			Release(ptr);
		});
		return true;
	}
	ptr = nullptr;
	return false;
}

bool KMeshManager::UpdateUtility(const KMeshUtilityInfoPtr& info, KMeshRef& ref)
{
	if (ref)
	{
		m_Device->Wait();
		return (*ref)->UpdateUtility(info, m_Device);
	}
	return false;
}

bool KMeshManager::AcquireOCQuery(std::vector<IKQueryPtr>& queries)
{
	ReleaseOCQuery(queries);
	queries.resize(KRenderGlobal::NumFramesInFlight);
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