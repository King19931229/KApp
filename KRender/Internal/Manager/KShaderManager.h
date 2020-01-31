#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKShader.h"

#include <unordered_map>

class KShaderManager
{
protected:
	struct ShaderUsingInfo
	{
		size_t useCount;
		IKShaderPtr shader;
	};

	typedef std::unordered_map<std::string, ShaderUsingInfo> ShaderMap;
	ShaderMap m_Shaders;
	IKRenderDevice* m_Device;
public:
	KShaderManager();
	~KShaderManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Reload();

	bool Acquire(const char* path, IKShaderPtr& shader, bool async);
	bool Release(IKShaderPtr& shader);
};