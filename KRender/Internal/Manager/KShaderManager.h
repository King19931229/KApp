#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKShader.h"

#include "glslang/Public/ShaderLang.h"
#include "SPIRV/GlslangToSpv.h"

#include <unordered_map>

class KSpirvBuiltInResource : public TBuiltInResource
{
public:
	KSpirvBuiltInResource();
	~KSpirvBuiltInResource();
};

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

	KSpirvBuiltInResource m_SpirVBuiltIn;
	IKRenderDevice* m_Device;
public:
	KShaderManager();
	~KShaderManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Reload();

	bool Acquire(ShaderType type, const char* path, IKShaderPtr& shader, bool async);
	bool Release(IKShaderPtr& shader);

	inline KSpirvBuiltInResource* GetSpirVBuildInResource() { return &m_SpirVBuiltIn; }
};