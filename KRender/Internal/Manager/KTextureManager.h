#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKTexture.h"
#include "Internal/VirtualTexture/KVirtualTexture.h"
#include <unordered_map>

class KTextureManager
{
protected:
	typedef std::unordered_map<std::string, KTextureRef> TextureMap;
	typedef std::unordered_map<size_t, KTextureRef> AnonymousTextureMap;
	typedef std::unordered_map<std::string, KVirtualTextureResourceRef> VirutalTextureMap;
	TextureMap m_Textures;
	AnonymousTextureMap m_AnonymousTextures;
	VirutalTextureMap m_VirtualTextures;
	KTextureRef m_ErrorTexture;
public:
	KTextureManager();
	~KTextureManager();

	bool Init();
	bool UnInit();

	bool Acquire(const char* path, uint32_t tileNum, KVirtualTextureResourceRef& ref, bool async);
	bool Acquire(const char* path, KTextureRef& ref, bool async);
	bool Acquire(const char* name, const void* pRawData, size_t dataLen, size_t width, size_t height, size_t depth, ImageFormat format, bool cubeMap, bool bGenerateMipmap, KTextureRef& ref, bool async);

	bool GetErrorTexture(KTextureRef& ref);
};