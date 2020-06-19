#pragma once
#include "Interface/IKMaterial.h"
#include <unordered_map>

class KMaterialManager
{
protected:
	struct MaterialUsingInfo
	{
		size_t useCount;
		IKMaterialPtr material;
	};
	typedef std::unordered_map<std::string, MaterialUsingInfo> MaterialMap;
	MaterialMap m_Materials;
	IKMaterialPtr m_MissingMaterial;
	IKRenderDevice* m_Device;
public:
	KMaterialManager();
	~KMaterialManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Acquire(const char* path, IKMaterialPtr& material, bool async);
	bool Release(IKMaterialPtr& material);

	bool GetMissingMaterial(IKMaterialPtr& material);
};