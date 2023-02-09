#include "KAssetLoader.h"
#include "Asset/KAssimpLoader.h"

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
	KAssimpLoader::Init();
	return true;
}
bool KAssetLoaderManager::UnInit()
{
	KAssimpLoader::UnInit();
	return true;
}

IKAssetLoaderPtr KAssetLoaderManager::GetLoader(const char* pFilePath)
{
	return IKAssetLoaderPtr(new KAssimpLoader());
}