#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include <unordered_map>

class KTextureManager
{
protected:
	struct TextureUsingInfo
	{
		size_t useCount;
		IKTexturePtr texture;
	};
	typedef std::unordered_map<std::string, TextureUsingInfo> TextureMap;
	IKTexturePtr m_ErrorTexture;
	IKSamplerPtr m_ErrorSampler;
	TextureMap m_Textures;
	IKRenderDevice* m_Device;
public:
	KTextureManager();
	~KTextureManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Acquire(const char* path, IKTexturePtr& texture, bool async);
	bool Release(IKTexturePtr& texture);
	bool GetErrorTexture(IKTexturePtr& texture);

	// for now
	bool CreateSampler(IKSamplerPtr& sampler);
	bool DestroySampler(IKSamplerPtr& sampler);
	bool GetErrorSampler(IKSamplerPtr& sampler);
};