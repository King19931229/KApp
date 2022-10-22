#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKShader.h"

#include "glslang/Public/ShaderLang.h"
#include <unordered_map>

class KSpirvBuiltInResource : public TBuiltInResource
{
public:
	KSpirvBuiltInResource();
	~KSpirvBuiltInResource();
};

struct KShaderCompileEnvironment
{
	std::vector<IKShader::MacroPair> macros;
	std::vector<IKShader::IncludeSource> includes;
};

class KShaderManager
{
protected:
	struct ShaderVariantionUsingInfo
	{
		size_t useCount;
		IKShaderPtr shader;
	};
	typedef std::unordered_map<size_t, ShaderVariantionUsingInfo> ShaderVariantionMap;
	typedef std::unordered_map<std::string, ShaderVariantionMap> ShaderMap;

	ShaderMap m_Shaders;

	KSpirvBuiltInResource m_SpirVBuiltIn;
	IKRenderDevice* m_Device;

	size_t CalcVariantionHash(const KShaderCompileEnvironment& env);
	bool AcquireImpl(ShaderType type, const char* path, const KShaderCompileEnvironment& env, IKShaderPtr& shader, bool async);
public:
	KShaderManager();
	~KShaderManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Reload();

	bool Acquire(ShaderType type, const char* path, IKShaderPtr& shader, bool async);
	bool Acquire(ShaderType type, const char* path, const KShaderCompileEnvironment& env, IKShaderPtr& shader, bool async);
	bool Release(IKShaderPtr& shader);

	inline KSpirvBuiltInResource* GetSpirVBuildInResource() { return &m_SpirVBuiltIn; }
};