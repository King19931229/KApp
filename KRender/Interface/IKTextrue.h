#pragma once
#include "Interface/IKRenderConfig.h"
#include <string>

struct IKTextrue
{
	virtual ~IKTextrue() {}
	virtual bool InitMemory(const std::string& filePath) = 0;
	virtual bool InitDevice() = 0;
	virtual TextureType GetTextureType() = 0;
};