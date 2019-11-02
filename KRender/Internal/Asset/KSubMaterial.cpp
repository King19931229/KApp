#include "KSubMaterial.h"

KSubMaterial::KSubMaterial(KMaterial* parent)
	: m_Parent(parent)
{

}

bool KSubMaterial::Init(const KMaterialTextrueBinding& textures)
{
	m_Textrues = textures;
	return true;
}

bool KSubMaterial::UnInit()
{
	m_Textrues.clear();
	return true;
}