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
	ASSERT_RESULT(m_Materials.empty());
}

bool KMaterialManager::Init(IKRenderDevice* device)
{
	m_Device = device;
	return true;
}

bool KMaterialManager::UnInit()
{
	for (auto it = m_Materials.begin(), itEnd = m_Materials.end(); it != itEnd; ++it)
	{
		MaterialUsingInfo& info = it->second;
		assert(info.material);
		info.material->UnInit();
	}
	m_Materials.clear();

	m_Device = nullptr;

	return true;
}

bool KMaterialManager::Acquire(const char* path, IKMaterialPtr& material, bool async)
{
	auto it = m_Materials.find(path);

	if (it != m_Materials.end())
	{
		MaterialUsingInfo& info = it->second;
		info.useCount += 1;
		material = info.material;
		return true;
	}

	material = IKMaterialPtr(KNEW KMaterial());

	if (material->InitFromFile(path, async))
	{
		MaterialUsingInfo info = { 1, material };
		m_Materials[path] = info;
		return true;
	}

	material = nullptr;
	return false;
}

bool KMaterialManager::Release(IKMaterialPtr& material)
{
	if (material)
	{
		auto it = m_Materials.find(material->GetPath());
		if (it != m_Materials.end())
		{
			MaterialUsingInfo& info = it->second;
			info.useCount -= 1;

			if (info.useCount == 0)
			{
				material->UnInit();
				m_Materials.erase(it);
			}

			material = nullptr;
			return true;
		}
	}
	return false;
}