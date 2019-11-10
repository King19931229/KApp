#pragma once
#include "Interface/IKAssetLoader.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

class KAssetLoader : public IKAssetLoader
{
protected:
	Assimp::Importer m_Importer;
	std::string m_AssetFolder;

	bool ImportAiScene(const aiScene* scene, const KAssetImportOption& importOption, KAssetImportResult& result);
public:
	virtual bool ImportFromMemory(const char* pData, size_t size, const KAssetImportOption& importOption, KAssetImportResult& result);
	virtual bool Import(const char* pszFile, const KAssetImportOption& importOption, KAssetImportResult& result);
};