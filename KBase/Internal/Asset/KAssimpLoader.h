#pragma once
#include "Interface/IKAssetLoader.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

class KAssimpLoader : public IKAssetLoader
{
protected:
	Assimp::Importer m_Importer;
	std::string m_AssetFolder;

	uint32_t GetFlags(const KAssetImportOption& importOption);
	bool ImportAiScene(const aiScene* scene, const KAssetImportOption& importOption, KMeshRawData& result);
public:
	KAssimpLoader();
	virtual ~KAssimpLoader();

	virtual bool ImportFromMemory(const char* pData, size_t size, const KAssetImportOption& importOption, KMeshRawData& result);
	virtual bool Import(const char* pszFile, const KAssetImportOption& importOption, KMeshRawData& result);

	static bool Init();
	static bool UnInit();
};