#include "KMaterialShader.h"
#include "Internal/KRenderGlobal.h"

static const size_t TANGENT_BINORMAL_INPUT_MACRO_INDEX = 0;
static const size_t DIFFUSE_SPECULAR_INPUT_INDEX = 1;
static const size_t UV2_INPUT_MACRO_INDEX = 2;
static const size_t BLEND_WEIGHT_INPUT_MACRO_INDEX = 3;

static const size_t MATERIAL_TEXTURE0_INDEX = 4;
static const size_t MATERIAL_TEXTURE1_INDEX = 5;
static const size_t MATERIAL_TEXTURE2_INDEX = 6;
static const size_t MATERIAL_TEXTURE3_INDEX = 7;

static const size_t MESHLET_INPUT_INDEX = 8;

static const char* INSTANCE_INPUT_MACRO = "INSTANCE_INPUT";

static const char* TANGENT_BINORMAL_INPUT_MACRO = "TANGENT_BINORMAL_INPUT";
static const char* DIFFUSE_SPECULAR_INPUT_MACRO = "DIFFUSE_SPECULAR_INPUT";
static const char* UV2_INPUT_MACRO = "UV2_INPUT";
static const char* BLEND_WEIGHT_INPUT_MACRO = "BLEND_WEIGHT_INPUT";

static const char* HAS_MATERIAL_TEXTURE0_MACRO = "HAS_MATERIAL_TEXTURE0";
static const char* HAS_MATERIAL_TEXTURE1_MACRO = "HAS_MATERIAL_TEXTURE1";
static const char* HAS_MATERIAL_TEXTURE2_MACRO = "HAS_MATERIAL_TEXTURE2";
static const char* HAS_MATERIAL_TEXTURE3_MACRO = "HAS_MATERIAL_TEXTURE3";

static const char* MESHLET_INPUT_MACRO = "MESHLET_INPUT";

// VS不需要贴图绑定宏
static const size_t VS_MACRO_SIZE = BLEND_WEIGHT_INPUT_MACRO_INDEX - TANGENT_BINORMAL_INPUT_MACRO_INDEX + 1;
// FS却需要用到顶点输入宏
static const size_t FS_MACRO_SIZE = MESHLET_INPUT_INDEX - TANGENT_BINORMAL_INPUT_MACRO_INDEX + 1;

static bool PERMUTATING_ARRAY_INIT = false;

size_t KMaterialShader::GenHash(const bool* macrosToEnable, size_t macrosSize)
{
	size_t hash = 0;
	for (size_t i = 0; i < macrosSize; ++i)
	{
		if (macrosToEnable[i])
		{
			hash |= 0x01;
		}
		hash <<= 1;
	}
	return hash;
}

size_t KMaterialShader::CalcHash(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding, bool meshletInput)
{
	ASSERT_RESULT(formats);
	ASSERT_RESULT(count);

	bool macrosToEnable[ARRAY_SIZE(PERMUTATING_MACRO)] = { false };
	// 其实没有必要 但是还是加上明确一下
	memset(macrosToEnable, 0, sizeof(macrosToEnable));

	for (size_t i = 0; i < count; ++i)
	{
		VertexFormat format = formats[i];

		if (format == VF_TANGENT_BINORMAL)
		{
			macrosToEnable[TANGENT_BINORMAL_INPUT_MACRO_INDEX] = true;
		}
		else if (format == VF_DIFFUSE_SPECULAR)
		{
			macrosToEnable[DIFFUSE_SPECULAR_INPUT_INDEX] = true;
		}
		else if (format == VF_UV2)
		{
			macrosToEnable[UV2_INPUT_MACRO_INDEX] = true;
		}
		else if (format == VF_BLEND_WEIGHTS_INDICES)
		{
			macrosToEnable[BLEND_WEIGHT_INPUT_MACRO_INDEX] = true;
		}
	}

	if (textureBinding)
	{
#define ASSIGN_TEXTURE_MACRO(SLOT) if (textureBinding->GetTexture(SLOT)) { macrosToEnable[MATERIAL_TEXTURE##SLOT##_INDEX] = true; }
		ASSIGN_TEXTURE_MACRO(0);
		ASSIGN_TEXTURE_MACRO(1);
		ASSIGN_TEXTURE_MACRO(2);
		ASSIGN_TEXTURE_MACRO(3);
#undef ASSIGN_TEXTURE_MACRO
	}

	if (meshletInput)
	{
		macrosToEnable[MESHLET_INPUT_INDEX] = true;
	}

	return GenHash(macrosToEnable, ARRAY_SIZE(PERMUTATING_MACRO));
}

std::mutex KMaterialShader::STATIC_RESOURCE_LOCK;
const char* KMaterialShader::PERMUTATING_MACRO[9];
KMaterialShader::MacrosMap KMaterialShader::VS_MACROS_MAP;
KMaterialShader::MacrosMap KMaterialShader::VS_INSTANCE_MACROS_MAP;
KMaterialShader::MacrosMap KMaterialShader::MS_MACROS_MAP;
KMaterialShader::MacrosMap KMaterialShader::FS_MACROS_MAP;

KMaterialShader::KMaterialShader()
	: m_Async(false)
{
	std::lock_guard<decltype(STATIC_RESOURCE_LOCK)> guard(STATIC_RESOURCE_LOCK);
	if (!PERMUTATING_ARRAY_INIT)
	{
		static_assert(SHADER_BINDING_MATERIAL_COUNT == MATERIAL_TEXTURE3_INDEX - MATERIAL_TEXTURE0_INDEX + 1, "count mush match");

		PERMUTATING_MACRO[TANGENT_BINORMAL_INPUT_MACRO_INDEX] = TANGENT_BINORMAL_INPUT_MACRO;
		PERMUTATING_MACRO[DIFFUSE_SPECULAR_INPUT_INDEX] = DIFFUSE_SPECULAR_INPUT_MACRO;
		PERMUTATING_MACRO[UV2_INPUT_MACRO_INDEX] = UV2_INPUT_MACRO;
		PERMUTATING_MACRO[BLEND_WEIGHT_INPUT_MACRO_INDEX] = BLEND_WEIGHT_INPUT_MACRO;

		PERMUTATING_MACRO[MATERIAL_TEXTURE0_INDEX] = HAS_MATERIAL_TEXTURE0_MACRO;
		PERMUTATING_MACRO[MATERIAL_TEXTURE1_INDEX] = HAS_MATERIAL_TEXTURE1_MACRO;
		PERMUTATING_MACRO[MATERIAL_TEXTURE2_INDEX] = HAS_MATERIAL_TEXTURE2_MACRO;
		PERMUTATING_MACRO[MATERIAL_TEXTURE3_INDEX] = HAS_MATERIAL_TEXTURE3_MACRO;

		PERMUTATING_MACRO[MESHLET_INPUT_INDEX] = MESHLET_INPUT_MACRO;

		bool macrosToEnable[ARRAY_SIZE(PERMUTATING_MACRO)] = { false };
		PermutateMacro(PERMUTATING_MACRO, macrosToEnable, ARRAY_SIZE(PERMUTATING_MACRO), VS_MACRO_SIZE, 0);

		PERMUTATING_ARRAY_INIT = true;
	}
}

KMaterialShader::~KMaterialShader()
{
	ASSERT_RESULT(m_VSShaderMap.empty());
	ASSERT_RESULT(m_VSInstanceShaderMap.empty());
	ASSERT_RESULT(m_FSShaderMap.empty());
}

void KMaterialShader::PermutateMacro(const char** marcosToPermutate,
	bool* macrosToEnable,
	size_t macrosSize,
	size_t vsMacrosSize,
	size_t permutateIndex
)
{
	ASSERT_RESULT(marcosToPermutate);
	ASSERT_RESULT(macrosToEnable);
	ASSERT_RESULT(permutateIndex <= macrosSize);

	if (permutateIndex == macrosSize)
	{
		Macros vsNoninstanceMacros;
		Macros vsInstanceMacros;
		Macros fsMacros;
		Macros msMacros;

		vsNoninstanceMacros.reserve(macrosSize + 1);
		vsInstanceMacros.reserve(macrosSize + 1);

		for (size_t i = 0; i < macrosSize; ++i)
		{
			if (macrosToEnable[i])
			{
				if (i < vsMacrosSize)
				{
					vsInstanceMacros.push_back({ marcosToPermutate[i], "1" });
					vsNoninstanceMacros.push_back({ marcosToPermutate[i], "1" });
					msMacros.push_back({ marcosToPermutate[i], "1" });
				}
				fsMacros.push_back({ marcosToPermutate[i], "1" });
			}
			else
			{
				if (i < vsMacrosSize)
				{
					vsInstanceMacros.push_back({ marcosToPermutate[i], "0" });
					vsNoninstanceMacros.push_back({ marcosToPermutate[i], "0" });
					msMacros.push_back({ marcosToPermutate[i], "0" });
				}
				fsMacros.push_back({ marcosToPermutate[i], "0" });
			}
		}

		vsNoninstanceMacros.push_back({ INSTANCE_INPUT_MACRO, "0" });
		vsInstanceMacros.push_back({ INSTANCE_INPUT_MACRO, "1" });
		msMacros.push_back({ INSTANCE_INPUT_MACRO, "0" });

		assert(MESHLET_INPUT_INDEX >= vsMacrosSize);
		vsNoninstanceMacros.push_back({ MESHLET_INPUT_MACRO, "0" });
		vsInstanceMacros.push_back({ MESHLET_INPUT_MACRO, "0" });
		msMacros.push_back({ MESHLET_INPUT_MACRO, "1" });

		size_t vsHash = GenHash(macrosToEnable, macrosSize);
		size_t fsHash = GenHash(macrosToEnable, macrosSize);

		VS_MACROS_MAP[vsHash] = std::make_shared<Macros>(std::move(vsNoninstanceMacros));
		VS_INSTANCE_MACROS_MAP[vsHash] = std::make_shared<Macros>(std::move(vsInstanceMacros));
		MS_MACROS_MAP[vsHash] = std::make_shared<Macros>(std::move(msMacros));
		FS_MACROS_MAP[fsHash] = std::make_shared<Macros>(std::move(fsMacros));

		return;
	}

	for (bool value : {false, true})
	{
		macrosToEnable[permutateIndex] = value;
		PermutateMacro(marcosToPermutate, macrosToEnable, macrosSize, vsMacrosSize, permutateIndex + 1);
	}
}

bool KMaterialShader::Init(const InitContext& context, bool async)
{
	UnInit();

	m_VSFile = context.vsFile;
	m_FSFile = context.fsFile;
	m_MSFile = context.msFile;

	m_Async = false;

	VertexFormat templateFormat[] = { VF_POINT_NORMAL_UV };

	m_VSTemplateShader = GetVSShader(templateFormat, ARRAY_SIZE(templateFormat));
	m_FSTemplateShader = GetFSShader(templateFormat, ARRAY_SIZE(templateFormat), nullptr, false);
	if(!m_MSFile.empty()) m_MSTemplateShader = GetMSShader(templateFormat, ARRAY_SIZE(templateFormat));

	m_Async = async;

	return true;
}

bool KMaterialShader::UnInit()
{
	m_VSTemplateShader = nullptr;
	m_FSTemplateShader = nullptr;
	m_MSTemplateShader = nullptr;

	m_VSFile.clear();
	m_FSFile.clear();
	m_MSFile.clear();

	for (ShaderMap* shadermap : { &m_VSShaderMap, &m_VSInstanceShaderMap, &m_FSShaderMap, &m_MSShaderMap })
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

bool KMaterialShader::Reload()
{
	for (ShaderMap* shadermap : { &m_VSShaderMap, &m_VSInstanceShaderMap, &m_FSShaderMap, &m_MSShaderMap })
	{
		for (auto& pair : *shadermap)
		{
			KShaderRef& shader = pair.second;
			(*shader)->Reload();
		}
	}
	return true;
}

bool KMaterialShader::IsInit()
{
	if (m_VSTemplateShader && m_FSTemplateShader)
	{
		return true;
	}
	return false;
}

bool KMaterialShader::IsAllVSLoaded()
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

bool KMaterialShader::IsVSTemplateLoaded()
{
	if (m_VSTemplateShader && m_VSTemplateShader->GetResourceState() == RS_DEVICE_LOADED)
	{
		return true;
	}
	return false;
}

bool KMaterialShader::IsFSTemplateLoaded()
{
	if (m_FSTemplateShader && m_FSTemplateShader->GetResourceState() == RS_DEVICE_LOADED)
	{
		return true;
	}
	return false;
}

bool KMaterialShader::IsAllFSLoaded()
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

bool KMaterialShader::IsBothLoaded(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding)
{
	size_t vsHash = CalcHash(formats, count, nullptr, false);
	if (m_VSShaderMap.find(vsHash) == m_VSShaderMap.end()) return false;
	if (m_VSInstanceShaderMap.find(vsHash) == m_VSInstanceShaderMap.end()) return false;

	size_t fsHash = CalcHash(formats, count, textureBinding, false);
	if (m_FSShaderMap.find(fsHash) == m_FSShaderMap.end()) return false;

	return true;
}

const KShaderInformation* KMaterialShader::GetVSInformation()
{
	if (m_VSTemplateShader)
	{
		return &m_VSTemplateShader->GetInformation();
	}
	return nullptr;
}

const KShaderInformation* KMaterialShader::GetFSInformation()
{
	if (m_FSTemplateShader)
	{
		return &m_FSTemplateShader->GetInformation();
	}
	return nullptr;
}

const KShaderInformation* KMaterialShader::GetMSInformation()
{
	if (m_MSTemplateShader)
	{
		return &m_MSTemplateShader->GetInformation();
	}
	return nullptr;
}

IKShaderPtr KMaterialShader::GetVSShader(const VertexFormat* formats, size_t count)
{
	if (formats && count)
	{
		size_t hash = CalcHash(formats, count, nullptr, false);
		auto it = m_VSShaderMap.find(hash);
		if (it != m_VSShaderMap.end())
		{
			return *it->second;
		}
		else
		{
			auto itMacro = VS_MACROS_MAP.find(hash);
			if (itMacro != VS_MACROS_MAP.end())
			{
				const Macros& macros = *itMacro->second;
				KShaderRef vsShader;

				KShaderCompileEnvironment env;
				env.macros = macros;

				if (KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, m_VSFile.c_str(), env, vsShader, m_Async))
				{
					m_VSShaderMap[hash] = vsShader;
					return *vsShader;
				}
			}
		}
	}
	return nullptr;
}

IKShaderPtr KMaterialShader::GetVSInstanceShader(const VertexFormat* formats, size_t count)
{
	if (formats && count)
	{
		size_t hash = CalcHash(formats, count, nullptr, false);
		auto it = m_VSInstanceShaderMap.find(hash);
		if (it != m_VSInstanceShaderMap.end())
		{
			return *it->second;
		}
		else
		{
			auto itMacro = VS_INSTANCE_MACROS_MAP.find(hash);
			if (itMacro != VS_INSTANCE_MACROS_MAP.end())
			{
				const Macros& macros = *itMacro->second;
				KShaderRef vsInstanceShader;

				KShaderCompileEnvironment env;
				env.macros = macros;

				if (KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, m_VSFile.c_str(), env, vsInstanceShader, m_Async))
				{
					m_VSInstanceShaderMap[hash] = vsInstanceShader;
					return *vsInstanceShader;
				}
			}
		}
	}
	return nullptr;
}

IKShaderPtr KMaterialShader::GetMSShader(const VertexFormat* formats, size_t count)
{
	if (formats && count)
	{
		size_t hash = CalcHash(formats, count, nullptr, false);
		auto it = m_MSShaderMap.find(hash);
		if (it != m_MSShaderMap.end())
		{
			return *it->second;
		}
		else
		{
			auto itMacro = MS_MACROS_MAP.find(hash);
			if (itMacro != MS_MACROS_MAP.end())
			{
				const Macros& macros = *itMacro->second;
				KShaderRef msShader;

				KShaderCompileEnvironment env;
				env.macros = macros;

				if (KRenderGlobal::ShaderManager.Acquire(ST_MESH, m_MSFile.c_str(), env, msShader, m_Async))
				{
					m_MSShaderMap[hash] = msShader;
					return *msShader;
				}
			}
		}
	}
	return nullptr;
}

IKShaderPtr KMaterialShader::GetFSShader(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding, bool meshletInput)
{
	if (formats && count)
	{
		size_t hash = CalcHash(formats, count, textureBinding, meshletInput);
		auto it = m_FSShaderMap.find(hash);
		if (it != m_FSShaderMap.end())
		{
			return *it->second;
		}
		else
		{
			auto itMacro = FS_MACROS_MAP.find(hash);
			if (itMacro != FS_MACROS_MAP.end())
			{
				const Macros& macros = *itMacro->second;
				KShaderRef fsShader;

				KShaderCompileEnvironment env;
				env.macros = macros;

				if (KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, m_FSFile.c_str(), env, fsShader, m_Async))
				{
					m_FSShaderMap[hash] = fsShader;
					return *fsShader;
				}
			}
		}
	}
	return nullptr;
}