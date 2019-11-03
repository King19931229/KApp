#include "KMaterial.h"
#include <assert.h>

KMaterial::KMaterial()
{
}

KMaterial::~KMaterial()
{
}

bool KMaterial::InitFromFile(const char* szPath)
{
	//TODO
	return true;
}

bool KMaterial::UnInit()
{
	return true;
}

KSubMaterialPtr KMaterial::GetSubMaterial(size_t mtlIndex)
{
	if(mtlIndex < m_SubMaterials.size())
	{
		return m_SubMaterials[mtlIndex];
	}
	assert(false && "material out of bound");
	return nullptr;
}