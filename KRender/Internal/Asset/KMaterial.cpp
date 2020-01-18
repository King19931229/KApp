#include "KMaterial.h"
#include "Internal/KRenderGlobal.h"

#include <assert.h>

KMaterial::KMaterial()
{
}

KMaterial::~KMaterial()
{
	assert(m_Textrues.empty());
}

bool KMaterial::InitFromFile(const char* szPath)
{
	return true;
}

bool KMaterial::UnInit()
{
	for(auto it = m_Textrues.begin(), itEnd = m_Textrues.end(); it != itEnd; ++it)
	{
		KMaterialTextureInfo& matTexInfo = it->second;

		IKTexturePtr& tex = matTexInfo.texture;
		assert(tex);
		KRenderGlobal::TextrueManager.Release(tex);

		IKSamplerPtr& sampler = matTexInfo.sampler;
		assert(sampler);
		KRenderGlobal::TextrueManager.DestroySampler(sampler);
	}
	m_Textrues.clear();
	return true;
}

bool KMaterial::ResignTexture(size_t slot, const char* path)
{
	IKTexturePtr texture;
	IKSamplerPtr sampler;

	if(KRenderGlobal::TextrueManager.Acquire(path, texture, true))
	{
		ASSERT_RESULT(KRenderGlobal::TextrueManager.CreateSampler(sampler));

		auto it = m_Textrues.find(slot);
		if(it != m_Textrues.end())
		{
			KMaterialTextureInfo& matTexInfo = it->second;

			IKTexturePtr& tex = matTexInfo.texture;
			assert(tex);
			KRenderGlobal::TextrueManager.Release(tex);

			IKSamplerPtr& sampler = matTexInfo.sampler;
			assert(sampler);
			KRenderGlobal::TextrueManager.DestroySampler(sampler);

			m_Textrues.erase(it);
		}

		sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
		ASSERT_RESULT(sampler->Init(texture, true));

		KMaterialTextureInfo info = { texture, sampler };
		m_Textrues[slot] = info;

		return true;
	}
	return false;
}

KMaterialTextureInfo KMaterial::GetTexture(size_t slot)
{
	KMaterialTextureInfo info = {nullptr, nullptr};
	auto it = m_Textrues.find(slot);
	if(it != m_Textrues.end())
	{
		info = it->second;
	}
	return info;
}