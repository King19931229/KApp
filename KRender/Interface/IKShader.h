#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKResource.h"
#include <string>
#include <vector>

struct IKShader : IKResource
{
	virtual ~IKShader() {}
	virtual bool SetConstantEntry(uint32_t constantID, uint32_t offset, size_t size, const void* data) = 0;
	virtual bool InitFromFile(const std::string& path, bool async) = 0;
	virtual bool InitFromString(const std::vector<char>& code, bool async) = 0;
	virtual bool UnInit() = 0;
	virtual const char* GetPath() = 0;
	virtual bool Reload() = 0;
};