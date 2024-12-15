#include "KMaterialManager.h"
#include "Internal/Asset/KMaterial.h"
#include "Internal/KRenderGlobal.h"

KMaterialManager::KMaterialManager()
{
}

KMaterialManager::~KMaterialManager()
{
	ASSERT_RESULT(!m_MissingMaterial);
	ASSERT_RESULT(m_Materials.empty());
}

bool KMaterialManager::Init()
{
	UnInit();
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
	m_Cache.Clear();
	return true;
}

bool KMaterialManager::Tick()
{
	m_Cache.Clear();
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

bool KMaterialManager::SetupMaterialGeneratedCode(const std::string& file, std::string& code)
{
	{
		auto it = m_Cache.materialGeneratedCode.find(file);
		if (it != m_Cache.materialGeneratedCode.end())
		{
			code = it->second;
			return true;
		}
	}

	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
	ASSERT_RESULT(system);
	IKSourceFilePtr materialSourceFile = GetSourceFile();
	materialSourceFile->SetIOHooker(IKSourceFile::IOHookerPtr(KNEW KShaderSourceHooker(system)));
	materialSourceFile->AddIncludeSource(KRenderGlobal::ShaderManager.GetBindingGenerateCode());
	if (materialSourceFile->Open(file.c_str()))
	{
		code = materialSourceFile->GetFinalSource();
		m_Cache.materialGeneratedCode.insert({file, code});
		return true;
	}
	else
	{
		code.clear();
		return false;
	}
}

bool KMaterialManager::Create(const KMeshRawData::Material& input, KMaterialRef& ref, bool async)
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
	else
	{
		material->UnInit();
		return false;
	}
}