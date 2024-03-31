#include "KShaderMap.h"
#include "Internal/KRenderGlobal.h"

static uint32_t INDEX_NONE = -1;

uint32_t VERTEX_FORMAT_MACRO_INDEX[VF_COUNT];
uint32_t MATERIAL_TEXTURE_BINDING_MACRO_INDEX[MAX_MATERIAL_TEXTURE_BINDING];
uint32_t VIRTUAL_TEXTURE_BINDING_MACRO_INDEX[MAX_MATERIAL_TEXTURE_BINDING];

static bool PERMUTATING_ARRAY_INIT = false;

size_t KShaderMap::GenHash(const bool* macrosToEnable)
{
	size_t hash = 0;
	for (size_t i = 0; i < MACRO_SIZE; ++i)
	{
		if (macrosToEnable[i])
		{
			hash |= 0x01;
		}
		hash <<= 1;
	}
	return hash;
}

size_t KShaderMap::CalcHash(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding)
{
	ASSERT_RESULT(!count || formats);

	bool macrosToEnable[MACRO_SIZE] = { false };
	memset(macrosToEnable, 0, sizeof(macrosToEnable));

	for (size_t i = 0; i < count; ++i)
	{
		VertexFormat format = formats[i];
		uint32_t index = VERTEX_FORMAT_MACRO_INDEX[format];
		if (index != INDEX_NONE)
		{
			macrosToEnable[index] = true;
		}
	}

	if (textureBinding)
	{
		for (uint32_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
		{
			if (textureBinding->GetTexture(i))
			{
				uint32_t index = 0;

				index = MATERIAL_TEXTURE_BINDING_MACRO_INDEX[i];
				macrosToEnable[index] = true;

				if (textureBinding->IsVirtual(i))
				{
					index = VIRTUAL_TEXTURE_BINDING_MACRO_INDEX[i];
					macrosToEnable[index] = true;
				}
			}
		}
	}

	EnsureMacroMap(macrosToEnable);
	return GenHash(macrosToEnable);
}

std::mutex KShaderMap::STATIC_RESOURCE_LOCK;
KShaderMap::MacrosMap KShaderMap::VS_MACROS_MAP;
KShaderMap::MacrosMap KShaderMap::VS_INSTANCE_MACROS_MAP;
KShaderMap::MacrosMap KShaderMap::VS_GPUSCENE_MACROS_MAP;
KShaderMap::MacrosMap KShaderMap::FS_MACROS_MAP;
KShaderMap::MacrosMap KShaderMap::FS_GPUSCENE_MACROS_MAP;
KShaderMap::MacrosSet KShaderMap::MACROS_SET;

void KShaderMap::InitializePermuationMap()
{
	std::lock_guard<decltype(STATIC_RESOURCE_LOCK)> guard(STATIC_RESOURCE_LOCK);
	if (!PERMUTATING_ARRAY_INIT)
	{
		memset(VERTEX_FORMAT_MACRO_INDEX, -1, sizeof(VERTEX_FORMAT_MACRO_INDEX));
		memset(MATERIAL_TEXTURE_BINDING_MACRO_INDEX, -1, sizeof(MATERIAL_TEXTURE_BINDING_MACRO_INDEX));
		memset(VIRTUAL_TEXTURE_BINDING_MACRO_INDEX, -1, sizeof(VIRTUAL_TEXTURE_BINDING_MACRO_INDEX));

		for (size_t i = 0; i < MACRO_SIZE; ++i)
		{
			if (PERMUTATING_MACRO[i].type == MT_HAS_VERTEX_INPUT)
			{
				VERTEX_FORMAT_MACRO_INDEX[PERMUTATING_MACRO[i].vertexFormat] = (uint32_t)i;
			}
			if (PERMUTATING_MACRO[i].type == MT_HAS_TEXTURE)
			{
				MATERIAL_TEXTURE_BINDING_MACRO_INDEX[PERMUTATING_MACRO[i].textureIndex] = (uint32_t)i;
			}
			if (PERMUTATING_MACRO[i].type == MT_HAS_VIRTUAL_TEXTURE)
			{
				VIRTUAL_TEXTURE_BINDING_MACRO_INDEX[PERMUTATING_MACRO[i].textureIndex] = (uint32_t)i;
			}
		}

		PERMUTATING_ARRAY_INIT = true;
	}
}

KShaderMap::KShaderMap()
	: m_Async(false)
{
}

KShaderMap::~KShaderMap()
{
	ASSERT_RESULT(m_VSShaderMap.empty());
	ASSERT_RESULT(m_VSInstanceShaderMap.empty());
	ASSERT_RESULT(m_VSGPUSceneShaderMap.empty());
	ASSERT_RESULT(m_FSShaderMap.empty());
}

void KShaderMap::EnsureMacroMap(const bool* macrosToEnable)
{
	std::lock_guard<decltype(STATIC_RESOURCE_LOCK)> guard(STATIC_RESOURCE_LOCK);

	size_t hash = GenHash(macrosToEnable);
	if (MACROS_SET.find(hash) == MACROS_SET.end())
	{
		MACROS_SET.insert(hash);

		Macros vsNoninstanceMacros;
		Macros vsInstanceMacros;
		Macros vsGPUSceneMacros;
		Macros fsMacros;
		Macros fsGPUSceneMacros;

		vsNoninstanceMacros.reserve(MACRO_SIZE);
		vsInstanceMacros.reserve(MACRO_SIZE);
		vsGPUSceneMacros.reserve(MACRO_SIZE);

		for (size_t i = 0; i < MACRO_SIZE; ++i)
		{
			static constexpr char* ENABLE = "1";
			static constexpr char* DISBLAE = "0";

			const char* pEnableOrDisable = nullptr;
			if (macrosToEnable[i])
			{
				pEnableOrDisable = ENABLE;
			}
			else
			{
				pEnableOrDisable = DISBLAE;
			}

			if (PERMUTATING_MACRO[i].type == MT_HAS_VERTEX_INPUT)
			{
				vsInstanceMacros.push_back({ PERMUTATING_MACRO[i].macro, pEnableOrDisable });
				vsNoninstanceMacros.push_back({ PERMUTATING_MACRO[i].macro, pEnableOrDisable });
				vsGPUSceneMacros.push_back({ PERMUTATING_MACRO[i].macro, pEnableOrDisable });
			}

			fsMacros.push_back({ PERMUTATING_MACRO[i].macro, pEnableOrDisable });
			fsGPUSceneMacros.push_back({ PERMUTATING_MACRO[i].macro, pEnableOrDisable });
		}

		vsNoninstanceMacros.push_back({ INSTANCE_INPUT_MACRO, "0" });
		vsInstanceMacros.push_back({ INSTANCE_INPUT_MACRO, "1" });

		vsGPUSceneMacros.push_back({ GPUSCENE_INPUT_MACRO, "1" });
		fsGPUSceneMacros.push_back({ GPUSCENE_INPUT_MACRO, "1" });

		VS_MACROS_MAP[hash] = std::make_shared<Macros>(std::move(vsNoninstanceMacros));
		VS_INSTANCE_MACROS_MAP[hash] = std::make_shared<Macros>(std::move(vsInstanceMacros));
		VS_GPUSCENE_MACROS_MAP[hash] = std::make_shared<Macros>(std::move(vsGPUSceneMacros));
		FS_MACROS_MAP[hash] = std::make_shared<Macros>(std::move(fsMacros));
		FS_GPUSCENE_MACROS_MAP[hash] = std::make_shared<Macros>(std::move(fsGPUSceneMacros));
	}
}

void KShaderMap::PermutateMacro(bool* macrosToEnable, size_t permutateIndex)
{
	ASSERT_RESULT(macrosToEnable);
	ASSERT_RESULT(permutateIndex <= MACRO_SIZE);

	if (permutateIndex == MACRO_SIZE)
	{
		EnsureMacroMap(macrosToEnable);
		return;
	}

	for (bool value : {false, true})
	{
		macrosToEnable[permutateIndex] = value;
		PermutateMacro(macrosToEnable, permutateIndex + 1);
	}
}

bool KShaderMap::Init(const KShaderMapInitContext& context, bool async)
{
	UnInit();

	m_VSFile = context.vsFile;
	m_FSFile = context.fsFile;
	m_Includes = context.IncludeSource;

	m_Async = false;

	VertexFormat templateFormat[] = { VF_POINT_NORMAL_UV };

	m_VSTemplateShader = GetVSShader(templateFormat, ARRAY_SIZE(templateFormat));
	m_FSTemplateShader = GetFSShader(templateFormat, ARRAY_SIZE(templateFormat), nullptr);

	m_Async = async;

	return true;
}

bool KShaderMap::UnInit()
{
	m_VSTemplateShader = nullptr;
	m_FSTemplateShader = nullptr;

	m_VSFile.clear();
	m_FSFile.clear();
	m_Includes.clear();

	for (ShaderMap* shadermap : { &m_VSShaderMap, &m_VSInstanceShaderMap, &m_FSShaderMap, &m_VSGPUSceneShaderMap, &m_FSGPUSceneShaderMap })
	{
		for (auto& pair : *shadermap)
		{
			KShaderRef& shader = pair.second;
			shader.Release();
		}
		shadermap->clear();
	}

	return true;
}

bool KShaderMap::Reload()
{
	for (ShaderMap* shadermap : { &m_VSShaderMap, &m_VSInstanceShaderMap, &m_FSShaderMap, &m_VSGPUSceneShaderMap, &m_FSGPUSceneShaderMap })
	{
		for (auto& pair : *shadermap)
		{
			KShaderRef& shader = pair.second;
			(*shader)->Reload();
		}
	};
	return true;
}

bool KShaderMap::IsInit()
{
	if (m_VSTemplateShader && m_FSTemplateShader)
	{
		return true;
	}
	return false;
}

bool KShaderMap::IsAllVSLoaded()
{
	if (!(m_VSShaderMap.empty() || m_VSInstanceShaderMap.empty()))
	{
		for (ShaderMap* shadermap : { &m_VSShaderMap, &m_VSInstanceShaderMap })
		{
			for (auto& pair : *shadermap)
			{
				KShaderRef& shader = pair.second;
				if ((*shader)->GetResourceState() != RS_DEVICE_LOADED)
				{
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool KShaderMap::IsVSTemplateLoaded()
{
	if (m_VSTemplateShader && m_VSTemplateShader->GetResourceState() == RS_DEVICE_LOADED)
	{
		return true;
	}
	return false;
}

bool KShaderMap::IsFSTemplateLoaded()
{
	if (m_FSTemplateShader && m_FSTemplateShader->GetResourceState() == RS_DEVICE_LOADED)
	{
		return true;
	}
	return false;
}

bool KShaderMap::IsAllFSLoaded()
{
	if (!(m_FSShaderMap.empty()))
	{
		for (ShaderMap* shadermap : { &m_FSShaderMap })
		{
			for (auto& pair : *shadermap)
			{
				KShaderRef& shader = pair.second;
				if ((*shader)->GetResourceState() != RS_DEVICE_LOADED)
				{
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool KShaderMap::IsPermutationLoaded(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding)
{
	size_t vsHash = CalcHash(formats, count, nullptr);
	if (m_VSShaderMap.find(vsHash) == m_VSShaderMap.end()) return false;
	if (m_VSInstanceShaderMap.find(vsHash) == m_VSInstanceShaderMap.end()) return false;

	size_t fsHash = CalcHash(formats, count, textureBinding);
	if (m_FSShaderMap.find(fsHash) == m_FSShaderMap.end()) return false;

	return true;
}

const KShaderInformation* KShaderMap::GetVSInformation()
{
	if (m_VSTemplateShader)
	{
		return &m_VSTemplateShader->GetInformation();
	}
	return nullptr;
}

const KShaderInformation* KShaderMap::GetFSInformation()
{
	if (m_FSTemplateShader)
	{
		return &m_FSTemplateShader->GetInformation();
	}
	return nullptr;
}

IKShaderPtr KShaderMap::GetShaderImplement(ShaderType shaderType, ShaderMap& shaderMap, MacrosMap& marcoMap, const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding)
{
	if (formats && count && (shaderType == ST_VERTEX || shaderType == ST_FRAGMENT))
	{
		size_t hash = CalcHash(formats, count, textureBinding);
		auto it = shaderMap.find(hash);
		if (it != shaderMap.end())
		{
			return *it->second;
		}
		else
		{
			auto itMacro = marcoMap.find(hash);
			if (itMacro != marcoMap.end())
			{
				const Macros& macros = *itMacro->second;
				KShaderRef shader;

				KShaderCompileEnvironment env;
				env.macros = macros;
				env.includes = m_Includes;

				const std::string& shaderFile = (shaderType == ST_VERTEX) ? m_VSFile.c_str() : m_FSFile.c_str();

				if (KRenderGlobal::ShaderManager.Acquire(shaderType, shaderFile.c_str(), env, shader, m_Async))
				{
					shaderMap[hash] = shader;
					return *shader;
				}
			}
		}
	}
	return nullptr;
}

IKShaderPtr KShaderMap::GetVSShader(const VertexFormat* formats, size_t count)
{
	return GetShaderImplement(ST_VERTEX, m_VSShaderMap, VS_MACROS_MAP, formats, count, nullptr);
}

IKShaderPtr KShaderMap::GetVSInstanceShader(const VertexFormat* formats, size_t count)
{
	return GetShaderImplement(ST_VERTEX, m_VSInstanceShaderMap, VS_INSTANCE_MACROS_MAP, formats, count, nullptr);
}

IKShaderPtr KShaderMap::GetVSGPUSceneShader(const VertexFormat* formats, size_t count)
{
	return GetShaderImplement(ST_VERTEX, m_VSGPUSceneShaderMap, VS_GPUSCENE_MACROS_MAP, formats, count, nullptr);
}

IKShaderPtr KShaderMap::GetFSShader(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding)
{
	return GetShaderImplement(ST_FRAGMENT, m_FSShaderMap, FS_MACROS_MAP, formats, count, textureBinding);
}

IKShaderPtr KShaderMap::GetFSGPUSceneShader(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding)
{
	return GetShaderImplement(ST_FRAGMENT, m_FSGPUSceneShaderMap, FS_GPUSCENE_MACROS_MAP, formats, count, textureBinding);
}