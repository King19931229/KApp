#pragma once
#include "Interface/IKMaterial.h"
#include <unordered_map>

class KMaterialManager
{
protected:
	typedef std::unordered_map<std::string, KMaterialRef> MaterialMap;
	MaterialMap m_Materials;
	KMaterialRef m_MissingMaterial;
	IKRenderDevice* m_Device;
public:
	KMaterialManager();
	~KMaterialManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Acquire(const char* path, KMaterialRef& ref, bool async);
	bool GetMissingMaterial(KMaterialRef& ref);

	bool Create(const KAssetImportResult::Material& input, KMaterialRef& ref, bool async);
};