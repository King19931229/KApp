#pragma once
#include "Interface/IKRenderConfig.h"
#include <string>

struct IKTexture
{
	virtual ~IKTexture() {}
	virtual bool InitMemory(const std::string& filePath) = 0;
	virtual bool InitDevice() = 0;
	virtual bool UnInit() = 0;

	virtual size_t GetWidth() = 0;
	virtual size_t GetHeight() = 0;
	virtual size_t GetDepth() = 0;
	virtual TextureType GetTextureType() = 0;
	virtual ElementFormat GetTextureFormat() = 0;
};