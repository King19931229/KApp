#include "KShaderManager.h"
#include "KBase/Publish/KHash.h"
#include "Internal/KRenderGlobal.h"

KSpirvBuiltInResource::KSpirvBuiltInResource()
{
	maxLights = 32;
	maxClipPlanes = 6;
	maxTextureUnits = 32;
	maxTextureCoords = 32;
	maxVertexAttribs = 64;
	maxVertexUniformComponents = 4096;
	maxVaryingFloats = 64;
	maxVertexTextureImageUnits = 32;
	maxCombinedTextureImageUnits = 80;
	maxTextureImageUnits = 32;
	maxFragmentUniformComponents = 4096;
	maxDrawBuffers = 32;
	maxVertexUniformVectors = 128;
	maxVaryingVectors = 8;
	maxFragmentUniformVectors = 16;
	maxVertexOutputVectors = 16;
	maxFragmentInputVectors = 15;
	minProgramTexelOffset = -8;
	maxProgramTexelOffset = 7;
	maxClipDistances = 8;
	maxComputeWorkGroupCountX = 65535;
	maxComputeWorkGroupCountY = 65535;
	maxComputeWorkGroupCountZ = 65535;
	maxComputeWorkGroupSizeX = 1024;
	maxComputeWorkGroupSizeY = 1024;
	maxComputeWorkGroupSizeZ = 64;
	maxComputeUniformComponents = 1024;
	maxComputeTextureImageUnits = 16;
	maxComputeImageUniforms = 8;
	maxComputeAtomicCounters = 8;
	maxComputeAtomicCounterBuffers = 1;
	maxVaryingComponents = 60;
	maxVertexOutputComponents = 64;
	maxGeometryInputComponents = 64;
	maxGeometryOutputComponents = 128;
	maxFragmentInputComponents = 128;
	maxImageUnits = 8;
	maxCombinedImageUnitsAndFragmentOutputs = 8;
	maxCombinedShaderOutputResources = 8;
	maxImageSamples = 0;
	maxVertexImageUniforms = 0;
	maxTessControlImageUniforms = 0;
	maxTessEvaluationImageUniforms = 0;
	maxGeometryImageUniforms = 0;
	maxFragmentImageUniforms = 8;
	maxCombinedImageUniforms = 8;
	maxGeometryTextureImageUnits = 16;
	maxGeometryOutputVertices = 256;
	maxGeometryTotalOutputComponents = 1024;
	maxGeometryUniformComponents = 1024;
	maxGeometryVaryingComponents = 64;
	maxTessControlInputComponents = 128;
	maxTessControlOutputComponents = 128;
	maxTessControlTextureImageUnits = 16;
	maxTessControlUniformComponents = 1024;
	maxTessControlTotalOutputComponents = 4096;
	maxTessEvaluationInputComponents = 128;
	maxTessEvaluationOutputComponents = 128;
	maxTessEvaluationTextureImageUnits = 16;
	maxTessEvaluationUniformComponents = 1024;
	maxTessPatchComponents = 120;
	maxPatchVertices = 32;
	maxTessGenLevel = 64;
	maxViewports = 16;
	maxVertexAtomicCounters = 0;
	maxTessControlAtomicCounters = 0;
	maxTessEvaluationAtomicCounters = 0;
	maxGeometryAtomicCounters = 0;
	maxFragmentAtomicCounters = 8;
	maxCombinedAtomicCounters = 8;
	maxAtomicCounterBindings = 1;
	maxVertexAtomicCounterBuffers = 0;
	maxTessControlAtomicCounterBuffers = 0;
	maxTessEvaluationAtomicCounterBuffers = 0;
	maxGeometryAtomicCounterBuffers = 0;
	maxFragmentAtomicCounterBuffers = 1;
	maxCombinedAtomicCounterBuffers = 1;
	maxAtomicCounterBufferSize = 16384;
	maxTransformFeedbackBuffers = 4;
	maxTransformFeedbackInterleavedComponents = 64;
	maxCullDistances = 8;
	maxCombinedClipAndCullDistances = 8;
	maxSamples = 4;

	maxMeshOutputVerticesNV = 256;
	maxMeshOutputPrimitivesNV = 512;
	maxMeshWorkGroupSizeX_NV = 32;
	maxMeshWorkGroupSizeY_NV = 1;
	maxMeshWorkGroupSizeZ_NV = 1;
	maxTaskWorkGroupSizeX_NV = 32;
	maxTaskWorkGroupSizeY_NV = 1;
	maxTaskWorkGroupSizeZ_NV = 1;
	maxMeshViewCountNV = 4;

	maxMeshOutputVerticesEXT = 256;
	maxMeshOutputPrimitivesEXT = 512;
	maxMeshWorkGroupSizeX_EXT = 128;
	maxMeshWorkGroupSizeY_EXT = 128;
	maxMeshWorkGroupSizeZ_EXT = 128;
	maxTaskWorkGroupSizeX_EXT = 128;
	maxTaskWorkGroupSizeY_EXT = 128;
	maxTaskWorkGroupSizeZ_EXT = 128;
	maxMeshViewCountEXT = 4;

	// TODO compile-time glslang version check
	// maxDualSourceDrawBuffersEXT = 1;

	limits.nonInductiveForLoops = 1;
	limits.whileLoops = 1;
	limits.doWhileLoops = 1;
	limits.generalUniformIndexing = 1;
	limits.generalAttributeMatrixVectorIndexing = 1;
	limits.generalVaryingIndexing = 1;
	limits.generalSamplerIndexing = 1;
	limits.generalVariableIndexing = 1;
	limits.generalConstantMatrixVectorIndexing = 1;

	glslang::InitializeProcess();
}

KSpirvBuiltInResource::~KSpirvBuiltInResource()
{
	glslang::FinalizeProcess();
}

KShaderManager::KShaderManager()
{
}

KShaderManager::~KShaderManager()
{
	assert(m_Shaders.empty());
}

size_t KShaderManager::CalcVariantionHash(const KShaderCompileEnvironment& env)
{
	std::string fullString;
	for (const IKShader::MacroPair& macroPair : env.macros)
	{
		fullString += std::get<0>(macroPair) + std::get<1>(macroPair);
	}
	for (const IKShader::IncludeSource& includePair : env.includeSources)
	{
		fullString += std::get<0>(includePair);
	}
	for (const IKShader::IncludeFile& includePair : env.includeFiles)
	{
		fullString += std::get<0>(includePair);
	}
	fullString += env.enableSourceDebug ? "D1" : "D0";

	size_t hash = env.parentEnv ? CalcVariantionHash(*env.parentEnv) : 0;
	KHash::HashCombine(hash, KHash::BKDR(fullString.c_str(), fullString.length()));
	return hash;
}

bool KShaderManager::Init()
{
	UnInit();

	std::string bindingCode;
	
	#define VERTEX_SEMANTIC(SEMANTIC, LOCATION)		bindingCode += std::string("#define ") + #SEMANTIC + " " + #LOCATION + "\n";
	#define CONSTANT_BUFFER_BINDING(CONSTANT)		bindingCode += std::string("#define ") + "BINDING_" + #CONSTANT + " " + std::to_string(CBT_##CONSTANT) + "\n";
	#define STORAGE_BUFFER_BINDING(STORAGE)			bindingCode += std::string("#define ") + "BINDING_" + #STORAGE + " " + std::to_string(SBT_##STORAGE) + "\n";
	#define TEXTURE_BINDING(SLOT)					bindingCode += std::string("#define ") + "BINDING_TEXTURE" + #SLOT + " " + std::to_string(TB_BEGIN + SLOT) + "\n";

	#include "Interface/KVertexSemantic.inl"
	#include "Interface/KShaderBinding.inl"

	#undef TEXTURE_BINDING
	#undef STORAGE_BINDING
	#undef CONSTANT_BUFFER_BINDING
	#undef VERTEX_SEMANTIC

	m_BindingInclude = std::make_pair("binding_generate_code.h", bindingCode);
	m_SourceFileIOHooker = IKSourceFile::IOHookerPtr(KNEW KShaderSourceHooker(KFileSystem::Manager->GetFileSystem(FSD_SHADER)));

	return true;
}

bool KShaderManager::UnInit()
{
	for (auto it = m_Shaders.begin(), itEnd = m_Shaders.end(); it != itEnd; ++it)
	{
		ShaderVariantionMap& variantionMap = it->second;
		for (auto it = variantionMap.begin(), itEnd = variantionMap.end(); it != itEnd; ++it)
		{
			KShaderRef& ref = it->second;
			ASSERT_RESULT(ref.GetRefCount() == 1);
		}
		variantionMap.clear();
	}
	m_Shaders.clear();
	m_SourceFileIOHooker = nullptr;
	return true;
}

bool KShaderManager::Reload()
{
	ClearSourceCache();
	for (auto it = m_Shaders.begin(), itEnd = m_Shaders.end(); it != itEnd; ++it)
	{
		ShaderVariantionMap& variantionMap = it->second;
		for (auto it = variantionMap.begin(), itEnd = variantionMap.end(); it != itEnd; ++it)
		{
			KShaderRef& ref = it->second;
			ASSERT_RESULT((*ref)->Reload());
		}
	}
	return true;
}

bool KShaderManager::ClearSourceCache()
{
	if (m_SourceFileIOHooker)
	{
		m_SourceFileIOHooker->ClearCache();
	}
	return true;
}

void KShaderManager::ApplyEnvironment(IKShaderPtr soul, const KShaderCompileEnvironment& env)
{
	if (soul)
	{
		if (env.parentEnv)
		{
			ApplyEnvironment(soul, *env.parentEnv);
		}
		for (const IKShader::MacroPair& macroPair : env.macros)
		{
			soul->AddMacro(macroPair);
		}
		for (const IKShader::IncludeSource& includePair : env.includeSources)
		{
			soul->AddIncludeSource(includePair);
		}
		for (const IKShader::IncludeFile& includePair : env.includeFiles)
		{
			soul->AddIncludeFile(includePair);
		}
		soul->SetSourceDebugEnable(env.enableSourceDebug);
	}
}

bool KShaderManager::AcquireByEnvironment(ShaderType type, const char* path, const KShaderCompileEnvironment& env, KShaderRef& shader, bool async)
{
	auto it = m_Shaders.find(path);
	if (it == m_Shaders.end())
	{
		it = m_Shaders.insert({ path, ShaderVariantionMap() }).first;
	}

	ShaderVariantionMap& variantionMap = it->second;
	size_t hash = CalcVariantionHash(env);

	auto itVar = variantionMap.find(hash);
	if (itVar == variantionMap.end())
	{
		IKShaderPtr soul;
		KRenderGlobal::RenderDevice->CreateShader(soul);
		ApplyEnvironment(soul, env);
		if (soul->InitFromFile(type, path, async))
		{
			KShaderRef ref(soul, [this](IKShaderPtr soul)
			{
				Release(soul);
			});
			variantionMap[hash] = ref;
			shader = ref;
			return true;
		}
	}
	else
	{
		shader = itVar->second;
		assert((*shader)->GetType() == type);
		assert(strcmp((*shader)->GetPath(), path) == 0);
		return true;
	}
	return false;
}

bool KShaderManager::Acquire(ShaderType type, const char* path, const KShaderCompileEnvironment& env, KShaderRef& shader, bool async)
{
	return AcquireByEnvironment(type, path, env, shader, async);
}

bool KShaderManager::Acquire(ShaderType type, const char* path, KShaderRef& shader, bool async)
{
	KShaderCompileEnvironment env;
	env.enableSourceDebug = true;
	return AcquireByEnvironment(type, path, env, shader, async);
}

bool KShaderManager::Release(IKShaderPtr& shader)
{
	if (shader)
	{
		KRenderGlobal::RenderDevice->Wait();
		shader->UnInit();
		shader = nullptr;
	}
	return true;
}