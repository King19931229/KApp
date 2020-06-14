#include "KShaderManager.h"
#include "KBase/Publish/KHash.h"
#include <assert.h>

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
	: m_Device(nullptr)
{
}

KShaderManager::~KShaderManager()
{
	assert(m_Shaders.empty());
}

size_t KShaderManager::CalcVariantionHash(const std::vector<IKShader::MacroPair>& macros)
{
	std::string fullString;
	for (const IKShader::MacroPair& macroPair : macros)
	{
		fullString += std::get<0>(macroPair) + std::get<1>(macroPair);
	}
	return KHash::BKDR(fullString.c_str(), fullString.length());
}

bool KShaderManager::Init(IKRenderDevice* device)
{
	UnInit();
	m_Device = device;
	return true;
}

bool KShaderManager::UnInit()
{
	// ASSERT_RESULT(m_Shaders.empty());
	for (auto it = m_Shaders.begin(), itEnd = m_Shaders.end(); it != itEnd; ++it)
	{
		ShaderVariantionMap& variantionMap = it->second;
		for (auto itVar = variantionMap.begin(), itVarEnd = variantionMap.end();
			itVar != itVarEnd; ++itVar)
		{
			assert(itVar->second.shader);
			SAFE_UNINIT(itVar->second.shader);
		}
		variantionMap.clear();
	}
	m_Shaders.clear();
	m_Device = nullptr;
	return true;
}

bool KShaderManager::Reload()
{
	for (auto it = m_Shaders.begin(), itEnd = m_Shaders.end(); it != itEnd; ++it)
	{
		ShaderVariantionMap& variantionMap = it->second;
		for (auto itVar = variantionMap.begin(), itVarEnd = variantionMap.end();
			itVar != itVarEnd; ++itVar)
		{
			assert(itVar->second.shader);
			itVar->second.shader->Reload();
		}
	}
	return true;
}

bool KShaderManager::AcquireImpl(ShaderType type, const char* path, const std::vector<IKShader::MacroPair>& macros, IKShaderPtr& shader, bool async)
{
	auto it = m_Shaders.find(path);
	if (it == m_Shaders.end())
	{
		it = m_Shaders.insert({ path, ShaderVariantionMap() }).first;
	}

	ShaderVariantionMap& variantionMap = it->second;
	size_t hash = CalcVariantionHash(macros);

	auto itVar = variantionMap.find(hash);
	if (itVar == variantionMap.end())
	{
		m_Device->CreateShader(shader);
		for (const IKShader::MacroPair& macroPair : macros)
		{
			shader->AddMacro(macroPair);
		}		
		if (shader->InitFromFile(type, path, async))
		{
			ShaderVariantionUsingInfo info = { 1, shader };
			variantionMap[hash] = info;
			return true;
		}
	}
	else
	{
		ShaderVariantionUsingInfo& info = itVar->second;
		info.useCount += 1;
		shader = info.shader;
		assert(shader->GetType() == type);
		assert(strcmp(shader->GetPath(), path) == 0);
		return true;
	}

	shader = nullptr;
	return false;
}

bool KShaderManager::Acquire(ShaderType type, const char* path, IKShaderPtr& shader, bool async)
{
	return AcquireImpl(type, path, {}, shader, async);
}

bool KShaderManager::Acquire(ShaderType type, const char* path, const std::vector<IKShader::MacroPair>& macros, IKShaderPtr& shader, bool async)
{
	return AcquireImpl(type, path, macros, shader, async);
}

bool KShaderManager::Release(IKShaderPtr& shader)
{
	if (shader)
	{
		auto it = m_Shaders.find(shader->GetPath());
		if (it != m_Shaders.end())
		{
			ShaderVariantionMap& variantionMap = it->second;

			std::vector<IKShader::MacroPair> macros;
			shader->GetAllMacro(macros);
			size_t hash = CalcVariantionHash(macros);

			auto itVar = variantionMap.find(hash);
			if (itVar != variantionMap.end())
			{
				ShaderVariantionUsingInfo& info = itVar->second;
				assert(shader == info.shader);
				info.useCount -= 1;
				if (info.useCount == 0)
				{
					m_Device->Wait();
					shader->UnInit();
					variantionMap.erase(itVar);
				}

				if (variantionMap.empty())
				{
					m_Shaders.erase(it);
				}

				shader = nullptr;
				return true;
			}
		}
	}
	return false;
}