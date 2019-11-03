#pragma once
#include "Interface/IKRenderConfig.h"
#include "KSubMaterial.h"

class KMaterial
{
protected:
	std::vector<KSubMaterialPtr> m_SubMaterials;
public:
	KMaterial();
	~KMaterial();

	bool InitFromFile(const char* szPath);
	bool UnInit();

	KSubMaterialPtr GetSubMaterial(size_t mtlIndex);
};

typedef std::shared_ptr<KMaterial> KMaterialPtr;