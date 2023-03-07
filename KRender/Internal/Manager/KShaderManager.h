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
	typedef std::unordered_map<size_t, KShaderRef> ShaderVariantionMap;
	typedef std::unordered_map<std::string, ShaderVariantionMap> ShaderMap;

	ShaderMap m_Shaders;

	KSpirvBuiltInResource m_SpirVBuiltIn;
	IKRenderDevice* m_Device;

	IKShader::IncludeSource m_BindingInclude;

	size_t CalcVariantionHash(const KShaderCompileEnvironment& env);
	bool AcquireByEnvironment(ShaderType type, const char* path, const KShaderCompileEnvironment& env, KShaderRef& shader, bool async);
	bool Release(IKShaderPtr& shader);
public:
	KShaderManager();
	~KShaderManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Reload();

	bool Acquire(ShaderType type, const char* path, KShaderRef& shader, bool async);
	bool Acquire(ShaderType type, const char* path, const KShaderCompileEnvironment& env, KShaderRef& shader, bool async);

	inline const KSpirvBuiltInResource* GetSpirVBuildInResource() const { return &m_SpirVBuiltIn; }
	inline const IKShader::IncludeSource& GetBindingGenerateCode() const { return m_BindingInclude; }
};