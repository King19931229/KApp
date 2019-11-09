#pragma once
#include "Interface/IKRenderConfig.h"
#include <string>
#include <vector>

struct IKShader
{
	virtual ~IKShader() {}
	virtual bool InitFromFile(const std::string path) = 0;
	virtual bool InitFromString(const std::vector<char> code) = 0;
	virtual bool UnInit() = 0;
	virtual const char* GetPath() = 0;
};