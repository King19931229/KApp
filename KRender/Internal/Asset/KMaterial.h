#pragma once
#include "Interface/IKRenderConfig.h"
#include <map>

typedef std::map<size_t, IKTexturePtr> KMaterialTextrueBinding;
class KMaterial
{
protected:
	KMaterialTextrueBinding m_Textrues;
public:
	KMaterial();
	~KMaterial();

	bool InitFromFile(const char* szPath);
	bool UnInit();
};

typedef std::shared_ptr<KMaterial> KMaterialPtr;