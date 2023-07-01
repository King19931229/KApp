#include "KMeshManager.h"
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

bool KMeshManager::AcquireImpl(const char* path, bool fromAsset, KMeshRef& ref)
{
	MeshInfo info = { std::string(path) };
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
		bRetValue = ptr->InitFromAsset(path);
	}
	else
	{
		bRetValue = ptr->InitFromFile(path);
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

bool KMeshManager::Acquire(const char* path, KMeshRef& ref)
{
	return AcquireImpl(path, false, ref);
}

bool KMeshManager::AcquireFromAsset(const char* path, KMeshRef& ref)
{
	return AcquireImpl(path, true, ref);
}

bool KMeshManager::New(KMeshRef& ref)
{
	KMeshPtr ptr = KMeshPtr(KNEW KMesh());
	ref = KMeshRef(ptr, [this](KMeshPtr ptr)
	{
		ptr->UnInit();
	});
	return true;
}

bool KMeshManager::AcquireFromUserData(const KMeshRawData& userData, const std::string& label, KMeshRef& ref)
{
	KMeshPtr ptr = KMeshPtr(KNEW KMesh());
	if (ptr->InitFromUserData(userData, label))
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