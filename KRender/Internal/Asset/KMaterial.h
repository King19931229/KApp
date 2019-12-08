#pragma once
#include "Interface/IKRenderConfig.h"
#include <map>

struct KMaterialTextureInfo
{
	IKTexturePtr texture;
	IKSamplerPtr sampler;

	bool IsComplete()
	{
		return texture != nullptr && sampler != nullptr;
	}
};

typedef std::map<size_t, KMaterialTextureInfo> KMaterialTextrueBinding;

enum MaterialTextureSlot
{
	MTS_DIFFUSE = 0,
	MTS_SPECULAR = 1,
	MTS_NORMAL = 2
};

class KMaterial
{
protected:
	KMaterialTextrueBinding m_Textrues;
public:
	KMaterial();
	~KMaterial();

	bool InitFromFile(const char* szPath);
	bool UnInit();

	bool ResignTexture(size_t slot, const char* path);
	KMaterialTextureInfo GetTexture(size_t slot);
};

typedef std::shared_ptr<KMaterial> KMaterialPtr;