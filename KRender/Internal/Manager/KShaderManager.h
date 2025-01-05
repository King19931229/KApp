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

class KShaderManager
{
protected:
	typedef std::unordered_map<size_t, KShaderRef> ShaderVariantionMap;
	typedef std::unordered_map<std::string, ShaderVariantionMap> ShaderMap;

	ShaderMap m_Shaders;
	KSpirvBuiltInResource m_SpirVBuiltIn;
	IKShader::IncludeSource m_BindingInclude;
	IKSourceFile::IOHookerPtr m_SourceFileIOHooker;

	size_t CalcVariantionHash(const KShaderCompileEnvironment& env);
	void ApplyEnvironment(IKShaderPtr soul, const KShaderCompileEnvironment& env);
	bool AcquireByEnvironment(ShaderType type, const char* path, const KShaderCompileEnvironment& env, KShaderRef& shader, bool async);
	bool Release(IKShaderPtr& shader);
public:
	KShaderManager();
	~KShaderManager();

	bool Init();
	bool UnInit();

	bool Reload();
	bool ClearSourceCache();

	bool Acquire(ShaderType type, const char* path, const KShaderCompileEnvironment& env, KShaderRef& shader, bool async);
	bool Acquire(ShaderType type, const char* path, KShaderRef& shader, bool async);

	inline const KSpirvBuiltInResource* GetSpirVBuildInResource() const { return &m_SpirVBuiltIn; }
	inline const IKShader::IncludeSource& GetBindingGenerateCode() const { return m_BindingInclude; }
	inline IKSourceFile::IOHookerPtr GetSourceFileIOHooker() { return m_SourceFileIOHooker; }
};