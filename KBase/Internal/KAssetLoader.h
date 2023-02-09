#pragma once
#include "Interface/IKAssetLoader.h"

class KAssetLoaderManager
{
public:
public:
	KAssetLoaderManager();
	~KAssetLoaderManager();

	static bool Init();
	static bool UnInit();

	static IKAssetLoaderPtr GetLoader(const char* pFilePath);
};