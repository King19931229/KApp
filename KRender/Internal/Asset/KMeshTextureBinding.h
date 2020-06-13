#pragma once
#include "Interface/IKRenderConfig.h"
#include <map>

enum MeshTextureSemantic
{
	MTS_DIFFUSE,
	MTS_SPECULAR,
	MTS_NORMAL,
	MTS_COUNT
};

struct KMeshTextureInfo
{
	IKTexturePtr texture;
	IKSamplerPtr sampler;

	bool IsComplete()
	{
		return texture != nullptr && sampler != nullptr;
	}
};

class KMeshTextureBinding
{
protected:
	KMeshTextureInfo m_Textures[MTS_COUNT + 1];
public:
	KMeshTextureBinding();
	~KMeshTextureBinding();

	bool Release();

	bool AssignTexture(MeshTextureSemantic semantic, const char* path);
	inline KMeshTextureInfo GetTexture(MeshTextureSemantic semantic) { return m_Textures[semantic]; }
};