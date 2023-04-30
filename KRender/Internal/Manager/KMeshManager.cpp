#include "KMeshManager.h"
#include "Internal/Asset/Utility/KMeshUtilityImpl.h"
#include "Internal/KRenderGlobal.h"
#include "Interface/IKQuery.h"

KMeshManager::KMeshManager()
{
}

KMeshManager::~KMeshManager()
{
	assert(m_Meshes.empty());
}

bool KMeshManager::Init()
{
	UnInit();
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
	return true;
}

bool KMeshManager::AcquireImpl(const char* path, bool fromAsset, bool hostVisible, KMeshRef& ref)
{
	MeshInfo info = { std::string(path), hostVisible };
	auto it = m_Meshes.find(info);

	if(it != m_Meshes.end())
	{
		ref = it->second;
		return true;
	}

	KMeshPtr ptr = KMeshPtr(KNEW KMesh());
	bool bRetValue = false;
	if(fromAsset)
	{
		bRetValue = ptr->InitFromAsset(path, hostVisible);
	}
	else
	{
		bRetValue = ptr->InitFromFile(path, hostVisible);
	}

	if(bRetValue)
	{
		ref = KMeshRef(ptr, [this](KMeshPtr ptr)
		{
			ptr->UnInit();
		});
		m_Meshes[info] = ref;
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

bool KMeshManager::AcquireAsUtility(const KMeshUtilityInfoPtr& info, KMeshRef& ref)
{
	KMeshPtr ptr = KMeshPtr(KNEW KMesh());
	if (ptr->InitUtility(info))
	{
		ref = KMeshRef(ptr, [this](KMeshPtr ptr)
		{
			ptr->UnInit();
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
		return ref->UpdateUtility(info);
	}
	return false;
}

bool KMeshManager::AcquireOCQuery(std::vector<IKQueryPtr>& queries)
{
	ReleaseOCQuery(queries);
	queries.resize(KRenderGlobal::NumFramesInFlight);
	for (IKQueryPtr& query : queries)
	{
		KRenderGlobal::RenderDevice->CreateQuery(query);
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