#include "KMaterialShader.h"
#include "Internal/KRenderGlobal.h"

static const char* PERMUTATING_MACRO[4];

static const size_t TANGENT_BINORMAL_INPUT_MACRO_INDEX = 0;
static const size_t DIFFUSE_SPECULAR_INPUT_INDEX = 1;
static const size_t UV2_INPUT_MACRO_INDEX = 2;
static const size_t BLEND_WEIGHT_INPUT_MACRO_INDEX = 3;

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

size_t KMaterialShader::CalcHash(const VertexFormat* formats, size_t count)
{
	ASSERT_RESULT(formats);
	ASSERT_RESULT(count);

	bool macrosToEnable[ARRAY_SIZE(PERMUTATING_MACRO)] = { false };
	// 其实没有必要 但是还是加上明确一下
	memset(macrosToEnable, 0, sizeof(macrosToEnable));

	for(size_t i = 0; i < count; ++i)
	{
		VertexFormat format = formats[i];
		// 这里敏捷一点
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

	return GenHash(macrosToEnable, ARRAY_SIZE(PERMUTATING_MACRO));
}

KMaterialShader::KMaterialShader()
{
	if (!PERMUTATING_ARRAY_INIT)
	{
		PERMUTATING_MACRO[TANGENT_BINORMAL_INPUT_MACRO_INDEX] = TANGENT_BINORMAL_INPUT_MACRO;
		PERMUTATING_MACRO[DIFFUSE_SPECULAR_INPUT_INDEX] = DIFFUSE_SPECULAR_INPUT_MACRO;
		PERMUTATING_MACRO[UV2_INPUT_MACRO_INDEX] = UV2_INPUT_MACRO;
		PERMUTATING_MACRO[BLEND_WEIGHT_INPUT_MACRO_INDEX] = BLEND_WEIGHT_INPUT_MACRO;
		PERMUTATING_ARRAY_INIT = true;
	}
}

KMaterialShader::~KMaterialShader()
{
	ASSERT_RESULT(m_VSShaderMap.empty());
	ASSERT_RESULT(m_VSInstanceShaderMap.empty());
	ASSERT_RESULT(m_FSShaderMap.empty());
}

void KMaterialShader::PermutateShader(const char** marcosToPermutate,
	bool* macrosToEnable,
	size_t macrosSize,
	size_t permutateIndex,
	bool async
)
{
	ASSERT_RESULT(marcosToPermutate);
	ASSERT_RESULT(macrosToEnable);
	ASSERT_RESULT(permutateIndex <= macrosSize);

	if (permutateIndex == macrosSize)
	{
		std::vector<IKShader::MacroPair> noninstanceMacros;
		std::vector<IKShader::MacroPair> instanceMacros;

		noninstanceMacros.reserve(macrosSize + 1);
		instanceMacros.reserve(macrosSize + 1);

		size_t hash = GenHash(macrosToEnable, macrosSize);
		for (size_t i = 0; i < macrosSize; ++i)
		{
			if (macrosToEnable[i])
			{
				noninstanceMacros.push_back({ marcosToPermutate[i], "1" });
			}
			else
			{
				noninstanceMacros.push_back({ marcosToPermutate[i], "0" });
			}
		}

		instanceMacros = noninstanceMacros;
		noninstanceMacros.push_back({ INSTANCE_INPUT_MACRO, "0" });
		instanceMacros.push_back({ INSTANCE_INPUT_MACRO, "1" });

		IKShaderPtr vsShader;
		IKShaderPtr vsInstanceShader;
		IKShaderPtr fsShader;

		if (KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, m_VSFile.c_str(), noninstanceMacros, vsShader, async) &&
			KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, m_VSFile.c_str(), instanceMacros, vsInstanceShader, async))
		{
			noninstanceMacros.pop_back();
			if (KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, m_FSFile.c_str(), noninstanceMacros, fsShader, async))
			{
				m_VSShaderMap.insert({ hash , vsShader });
				m_VSInstanceShaderMap.insert({ hash , vsInstanceShader });
				m_FSShaderMap.insert({ hash , fsShader });

				if (!m_VSTemplateShader)
				{
					m_VSTemplateShader = vsShader;
				}
				if (!m_FSTemplateShader)
				{
					m_FSTemplateShader = fsShader;
				}

				return;
			}
		}	

		if (vsShader) { KRenderGlobal::ShaderManager.Release(vsShader); }
		if (vsInstanceShader) { KRenderGlobal::ShaderManager.Release(vsInstanceShader); }
		if (fsShader) { KRenderGlobal::ShaderManager.Release(fsShader); }
		return;
	}

	for (bool value : {false, true})
	{
		macrosToEnable[permutateIndex] = value;
		PermutateShader(marcosToPermutate, macrosToEnable, macrosSize, permutateIndex + 1, async);
	}
}

bool KMaterialShader::Init(const std::string& vsFile, const std::string& fsFile, bool async)
{
	UnInit();

	m_VSFile = vsFile;
	m_FSFile = fsFile;

	bool macrosToEnable[ARRAY_SIZE(PERMUTATING_MACRO)] = { false };
	PermutateShader(PERMUTATING_MACRO, macrosToEnable, ARRAY_SIZE(PERMUTATING_MACRO), 0, async);
	return true;
}

bool KMaterialShader::UnInit()
{
	m_VSTemplateShader = nullptr;
	m_FSTemplateShader = nullptr;

	m_VSFile.clear();
	m_FSFile.clear();

	for (ShaderMap* shadermap : { &m_VSShaderMap, &m_VSInstanceShaderMap, &m_FSShaderMap })
	{
		for (auto& pair : *shadermap)
		{
			IKShaderPtr& shader = pair.second;
			KRenderGlobal::ShaderManager.Release(shader);
			shader = nullptr;
		}
		shadermap->clear();
	}

	return true;
}

bool KMaterialShader::Reload()
{
	for (ShaderMap* shadermap : { &m_VSShaderMap, &m_VSInstanceShaderMap, &m_FSShaderMap })
	{
		for (auto& pair : *shadermap)
		{
			IKShaderPtr& shader = pair.second;
			shader->Reload();
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

bool KMaterialShader::IsVSLoaded()
{
	if (!(m_VSShaderMap.empty() || m_VSInstanceShaderMap.empty()))
	{
		for (ShaderMap* shadermap : { &m_VSShaderMap, &m_VSInstanceShaderMap})
		{
			for (auto& pair : *shadermap)
			{
				IKShaderPtr& shader = pair.second;
				if (shader->GetResourceState() != RS_DEVICE_LOADED)
				{
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool KMaterialShader::IsFSLoaded()
{
	if (!(m_FSShaderMap.empty()))
	{
		for (ShaderMap* shadermap : { &m_FSShaderMap })
		{
			for (auto& pair : *shadermap)
			{
				IKShaderPtr& shader = pair.second;
				if (shader->GetResourceState() != RS_DEVICE_LOADED)
				{
					return false;
				}
			}
		}
		return true;
	}
	return false;
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

IKShaderPtr KMaterialShader::GetVSShader(const VertexFormat* formats, size_t count)
{
	if (formats && count)
	{
		size_t hash = CalcHash(formats, count);
		auto it = m_VSShaderMap.find(hash);
		if (it != m_VSShaderMap.end())
		{
			return it->second;
		}
	}
	return nullptr;
}

IKShaderPtr KMaterialShader::GetVSInstanceShader(const VertexFormat* formats, size_t count)
{
	if (formats && count)
	{
		size_t hash = CalcHash(formats, count);
		auto it = m_VSInstanceShaderMap.find(hash);
		if (it != m_VSInstanceShaderMap.end())
		{
			return it->second;
		}
	}
	return nullptr;
}

IKShaderPtr KMaterialShader::GetFSShader(const VertexFormat* formats, size_t count)
{
	if(formats && count)
	{
		size_t hash = CalcHash(formats, count);
		auto it = m_FSShaderMap.find(hash);
		if (it != m_FSShaderMap.end())
		{
			return it->second;
		}
	}
	return nullptr;
}