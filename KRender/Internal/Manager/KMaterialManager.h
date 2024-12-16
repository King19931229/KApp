#pragma once
#include "Interface/IKMaterial.h"
#include <unordered_map>

class KMaterialManager
{
protected:
	typedef std::unordered_map<std::string, KMaterialRef> MaterialMap;
	MaterialMap m_Materials;
	KMaterialRef m_MissingMaterial;
	struct Cache
	{
		std::unordered_map<std::string, std::string> materialGeneratedCode;
		void Clear()
		{
			materialGeneratedCode.clear();
		}
	} m_Cache;
public:
	KMaterialManager();
	~KMaterialManager();

	bool Init();
	bool UnInit();

	bool Tick();

	bool Acquire(const char* path, KMaterialRef& ref, bool async);
	bool GetMissingMaterial(KMaterialRef& ref);

	bool SetupMaterialGeneratedCode(const std::string& file, std::string& code);

	bool Create(const KMeshRawData::Material& input, KMaterialRef& ref, bool async);
};