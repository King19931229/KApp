#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKTexture.h"
#include <unordered_map>

class KTextureManager
{
protected:
	typedef std::unordered_map<std::string, KTextureRef> TextureMap;
	TextureMap m_Textures;
	KTextureRef m_ErrorTexture;
	IKRenderDevice* m_Device;

	bool Release(IKTexturePtr& texture);
public:
	KTextureManager();
	~KTextureManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Acquire(const char* path, KTextureRef& ref, bool async);
	bool GetErrorTexture(KTextureRef& ref);
};