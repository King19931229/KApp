#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKTexture.h"
#include <unordered_map>

class KTextureManager
{
protected:
	typedef std::unordered_map<std::string, KTextureRef> TextureMap;
	typedef std::unordered_map<size_t, KTextureRef> AnonymousTextureMap;
	TextureMap m_Textures;
	AnonymousTextureMap m_AnonymousTextures;
	KTextureRef m_ErrorTexture;
	IKRenderDevice* m_Device;

	bool Release(IKTexturePtr& texture);
public:
	KTextureManager();
	~KTextureManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Acquire(const char* path, KTextureRef& ref, bool async);
	bool Acquire(const void* pRawData, size_t dataLen, size_t width, size_t height, size_t depth, ImageFormat format, bool cubeMap, bool bGenerateMipmap, KTextureRef& ref, bool async);

	bool GetErrorTexture(KTextureRef& ref);
};