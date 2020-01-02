#pragma once
#include "Interface/IKRenderConfig.h"
#include "KBase/Interface/IKCodec.h"
#include <string>

struct IKTexture
{
	virtual ~IKTexture() {}
	virtual bool InitMemoryFromFile(const std::string& filePath, bool bGenerateMipmap) = 0;
	virtual bool InitMemoryFromData(const void* pRawData, size_t width, size_t height, ImageFormat format, bool bGenerateMipmap) = 0;
	virtual bool InitMemeoryAsRT(size_t width, size_t height, ElementFormat format) = 0;
	virtual bool InitDevice() = 0;
	virtual bool UnInit() = 0;

	virtual size_t GetWidth() = 0;
	virtual size_t GetHeight() = 0;
	virtual size_t GetDepth() = 0;
	virtual unsigned short GetMipmaps() = 0;

	virtual TextureType GetTextureType() = 0;
	virtual ElementFormat GetTextureFormat() = 0;

	virtual const char* GetPath() = 0;
};