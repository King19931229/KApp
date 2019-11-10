#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include <map>

class KTextureManager
{
protected:
	struct TextureUsingInfo
	{
		size_t useCount;
		IKTexturePtr texture;
	};
	typedef std::map<std::string, TextureUsingInfo> TextureMap;
	TextureMap m_Textures;
	IKRenderDevice* m_Device;
public:
	KTextureManager();
	~KTextureManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Acquire(const char* path, IKTexturePtr& texture);
	bool Release(IKTexturePtr& texture);

	// for now
	bool CreateSampler(IKSamplerPtr& sampler);
	bool DestroySampler(IKSamplerPtr& sampler);
};