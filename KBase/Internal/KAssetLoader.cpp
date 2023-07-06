#include "KAssetLoader.h"
#include "Asset/KAssimpLoader.h"
#include "Asset/KGLTFLoader.h"
#include "Publish/KFileTool.h"

namespace KAssetLoader
{
	bool CreateAssetLoaderManager()
	{
		KAssetLoaderManager::Init();
		return true;
	}

	bool DestroyAssetLoaderManager()
	{
		KAssetLoaderManager::UnInit();
		return true;
	}

	IKAssetLoaderPtr GetLoader(const char* pszFile)
	{
		return KAssetLoaderManager::GetLoader(pszFile);
	}
}

KAssetLoaderManager::KAssetLoaderManager()
{
}

KAssetLoaderManager::~KAssetLoaderManager()
{
}

bool KAssetLoaderManager::Init()
{
	bool bSuccess = true;
	bSuccess &= KAssimpLoader::Init();
	bSuccess &= KGLTFLoader::Init();
	return bSuccess;
}

bool KAssetLoaderManager::UnInit()
{
	bool bSuccess = true;
	bSuccess &= KAssimpLoader::UnInit();
	bSuccess &= KGLTFLoader::UnInit();
	return bSuccess;
}

IKAssetLoaderPtr KAssetLoaderManager::GetLoader(const char* pFilePath)
{
	std::string name, ext;
	if (KFileTool::SplitExt(pFilePath, name, ext))
	{
		std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
		if (ext == ".gltf" || ext == ".glb")
		{
			return IKAssetLoaderPtr(KNEW KGLTFLoader());
		}
	}
	return IKAssetLoaderPtr(KNEW KAssimpLoader());
}