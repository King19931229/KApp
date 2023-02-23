#include "KMaterialManager.h"
#include "Internal/Asset/KMaterial.h"
#include <assert.h>

KMaterialManager::KMaterialManager()
	: m_Device(nullptr)
{
}

KMaterialManager::~KMaterialManager()
{
	ASSERT_RESULT(m_Device == nullptr);
	ASSERT_RESULT(!m_MissingMaterial);
	ASSERT_RESULT(m_Materials.empty());
}

bool KMaterialManager::Init(IKRenderDevice* device)
{
	UnInit();
	m_Device = device;
	ASSERT_RESULT(Acquire("Materials/Missing.mtl", m_MissingMaterial, false));
	return true;
}

bool KMaterialManager::UnInit()
{
	m_MissingMaterial.Release();
	for (auto it = m_Materials.begin(), itEnd = m_Materials.end(); it != itEnd; ++it)
	{
		KMaterialRef& ref = it->second;
		ASSERT_RESULT(ref.GetRefCount() == 1);
	}
	m_Materials.clear();
	m_Device = nullptr;
	return true;
}

bool KMaterialManager::GetMissingMaterial(KMaterialRef& ref)
{
	if (m_MissingMaterial)
	{
		ref = m_MissingMaterial;
		return true;
	}
	return false;
}

bool KMaterialManager::Acquire(const char* path, KMaterialRef& ref, bool async)
{
	auto it = m_Materials.find(path);

	if (it != m_Materials.end())
	{
		ref = it->second;
		return true;
	}

	IKMaterialPtr material = IKMaterialPtr(KNEW KMaterial());

	if (material->InitFromFile(path, async))
	{
		ref = KMaterialRef(material, [this](IKMaterialPtr material)
		{
			material->UnInit();
		});
		m_Materials[path] = ref;
		return true;
	}

	return false;
}

bool KMaterialManager::Create(const KAssetImportResult::Material& input, KMaterialRef& ref, bool async)
{
	IKMaterialPtr material = IKMaterialPtr(KNEW KMaterial());
	if (material->InitFromImportAssetMaterial(input, async))
	{
		ref = KMaterialRef(material, [this](IKMaterialPtr material)
		{
			material->UnInit();
		});
		return true;
	}
	return false;
}