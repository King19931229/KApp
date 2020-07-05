#include "KMeshTextureBinding.h"
#include "Internal/KRenderGlobal.h"
#include <assert.h>

KMeshTextureBinding::KMeshTextureBinding()
{
}

KMeshTextureBinding::~KMeshTextureBinding()
{	
}

bool KMeshTextureBinding::Release()
{
	for (size_t i = 0; i < MTS_COUNT; ++i)
	{
		KMeshTextureInfo& info = m_Textures[i];

		if (info.texture)
		{
			KRenderGlobal::TextureManager.Release(info.texture);
			info.texture = nullptr;
		}
		if (info.sampler)
		{
			KRenderGlobal::TextureManager.DestroySampler(info.sampler);
			info.sampler = nullptr;
		}
	}

	return true;
}

bool KMeshTextureBinding::AssignTexture(MeshTextureSemantic semantic, const char* path)
{
	ASSERT_RESULT(semantic != MTS_COUNT);

	KMeshTextureInfo& info = m_Textures[semantic];

	if (info.texture)
	{
		KRenderGlobal::TextureManager.Release(info.texture);
		info.texture = nullptr;
	}
	if (info.sampler)
	{
		KRenderGlobal::TextureManager.DestroySampler(info.sampler);
		info.sampler = nullptr;
	}

	if(KRenderGlobal::TextureManager.Acquire(path, info.texture, true))
	{
		ASSERT_RESULT(KRenderGlobal::TextureManager.CreateSampler(info.sampler));

		info.sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
		ASSERT_RESULT(info.sampler->Init(info.texture, true));

		return true;
	}
	return false;
}