#pragma once
#include "Interface/IKShader.h"
#include "Interface/IKRenderCommand.h"
#include "Interface/IKMaterial.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>

static constexpr char* INSTANCE_INPUT_MACRO = "INSTANCE_INPUT";
static constexpr char* MESHLET_INPUT_MACRO = "MESHLET_INPUT";

struct KShaderMapMacro
{
	const char* macro;
	bool vsFormatMacro;
	VertexFormat vertexFormat;
	bool fsTextureMacro;
	uint32_t textureIndex;
};

static constexpr KShaderMapMacro PERMUTATING_MACRO[]
{
	// { macro, vsFormatMacro, vertexFormat, fsTextureMacro, textureIndex }
	{ "TANGENT_BINORMAL_INPUT", true,	VF_TANGENT_BINORMAL,		false,	-1 },
	{ "BLEND_WEIGHT_INPUT",		true,	VF_BLEND_WEIGHTS_INDICES,	false,	-1 },
	{ "UV2_INPUT",				true,	VF_UV2,						false,	-1 },

	{ "VERTEX_COLOR_INPUT0",	true,	VF_COLOR0,					false,	-1 },
	{ "VERTEX_COLOR_INPUT1",	true,	VF_COLOR1,					false,	-1 },
	{ "VERTEX_COLOR_INPUT2",	true,	VF_COLOR2,					false,	-1 },
	{ "VERTEX_COLOR_INPUT3",	true,	VF_COLOR3,					false,	-1 },
	{ "VERTEX_COLOR_INPUT4",	true,	VF_COLOR4,					false,	-1 },
	{ "VERTEX_COLOR_INPUT5",	true,	VF_COLOR5,					false,	-1 },

	{ "HAS_MATERIAL_TEXTURE0",	false,	VF_UNKNOWN,					true,	0 },
	{ "HAS_MATERIAL_TEXTURE1",	false,	VF_UNKNOWN,					true,	1 },
	{ "HAS_MATERIAL_TEXTURE2",	false,	VF_UNKNOWN,					true,	2 },
	{ "HAS_MATERIAL_TEXTURE3",	false,	VF_UNKNOWN,					true,	3 },
	{ "HAS_MATERIAL_TEXTURE4",	false,	VF_UNKNOWN,					true,	4 },
	{ "HAS_MATERIAL_TEXTURE5",	false,	VF_UNKNOWN,					true,	5 },
	{ "HAS_MATERIAL_TEXTURE6",	false,	VF_UNKNOWN,					true,	6 },
	{ "HAS_MATERIAL_TEXTURE7",	false,	VF_UNKNOWN,					true,	7 },

	{ "HAS_MATERIAL_TEXTURE8",	false,	VF_UNKNOWN,					true,	8 },
	{ "HAS_MATERIAL_TEXTURE9",	false,	VF_UNKNOWN,					true,	9 },
	{ "HAS_MATERIAL_TEXTURE10",	false,	VF_UNKNOWN,					true,	10 },
	{ "HAS_MATERIAL_TEXTURE11",	false,	VF_UNKNOWN,					true,	11 },
	{ "HAS_MATERIAL_TEXTURE12",	false,	VF_UNKNOWN,					true,	12 },
	{ "HAS_MATERIAL_TEXTURE13",	false,	VF_UNKNOWN,					true,	13 },
	{ "HAS_MATERIAL_TEXTURE14",	false,	VF_UNKNOWN,					true,	14 },
	{ "HAS_MATERIAL_TEXTURE15",	false,	VF_UNKNOWN,					true,	15 },
};

static constexpr size_t MACRO_SIZE = sizeof(PERMUTATING_MACRO) / sizeof(KShaderMapMacro);

extern uint32_t VERTEX_FORMAT_MACRO_INDEX[VF_COUNT];
extern uint32_t MATERIAL_TEXTURE_BINDING_MACRO_INDEX[MAX_MATERIAL_TEXTURE_BINDING];

class KTextureBinding
{
protected:
	IKTexturePtr m_TextureMap[MAX_MATERIAL_TEXTURE_BINDING];
public:
	KTextureBinding()
	{
	}

	~KTextureBinding()
	{
		Empty();
	}

	IKTexturePtr GetTexture(uint32_t slot) const
	{
		assert(slot < MAX_MATERIAL_TEXTURE_BINDING);
		return m_TextureMap[slot];
	}

	void AssignTexture(uint32_t slot, IKTexturePtr texture)
	{
		assert(slot < MAX_MATERIAL_TEXTURE_BINDING);
		m_TextureMap[slot] = texture;
	}

	void Empty()
	{
		for (size_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
		{
			m_TextureMap[i] = nullptr;
		}
	}

	void Init(IKMaterialTextureBinding* mtlTexBinding)
	{
		Empty();
		if (mtlTexBinding)
		{
			for (uint8_t i = 0; i < mtlTexBinding->GetNumSlot(); ++i)
			{
				AssignTexture(i, mtlTexBinding->GetTexture(i));
			}
		}
	}
};

struct KShaderMapInitContext
{
	std::string vsFile;
	std::string fsFile;
	std::string msFile;
	std::vector<std::tuple<std::string, std::string>> IncludeSource;
};

class KShaderMap
{
protected:
	typedef std::unordered_map<size_t, KShaderRef> ShaderMap;
	ShaderMap m_VSShaderMap;
	ShaderMap m_VSInstanceShaderMap;
	ShaderMap m_MSShaderMap;
	ShaderMap m_FSShaderMap;

	IKShaderPtr m_VSTemplateShader;
	IKShaderPtr m_FSTemplateShader;
	IKShaderPtr m_MSTemplateShader;

	std::string m_VSFile;
	std::string m_MSFile;
	std::string m_FSFile;
	std::vector<IKShader::IncludeSource> m_Includes;

	bool m_Async;

	typedef std::vector<IKShader::MacroPair> Macros;
	typedef std::shared_ptr<Macros> MacrosPtr;
	typedef std::unordered_map<size_t, MacrosPtr> MacrosMap;
	typedef std::unordered_set<size_t> MacrosSet;

	static std::mutex STATIC_RESOURCE_LOCK;
	static MacrosMap VS_MACROS_MAP;
	static MacrosMap VS_INSTANCE_MACROS_MAP;
	static MacrosMap MS_MACROS_MAP;
	static MacrosMap FS_MACROS_MAP;
	static MacrosSet MACROS_SET;

	static size_t GenHash(const bool* macrosToEnable);
	static size_t CalcHash(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding);

	static void EnsureMacroMap(const bool* macrosToEnable);
	static void PermutateMacro(bool* macrosToEnable, size_t permutateIndex);
public:
	KShaderMap();
	~KShaderMap();

	static void InitializePermuationMap();

	bool Init(const KShaderMapInitContext& context, bool async);
	bool UnInit();
	bool Reload();

	bool IsInit();

	bool IsAllVSLoaded();
	bool IsAllFSLoaded();
	bool IsVSTemplateLoaded();
	bool IsFSTemplateLoaded();
	bool IsBothLoaded(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding);

	const std::string& GetVSPath() const { return m_VSFile; }
	const std::string& GetMSPath() const { return m_MSFile; }
	const std::string& GetFSPath() const { return m_FSFile; }

	bool HasMSShader() const { return !m_MSFile.empty(); }

	const KShaderInformation* GetVSInformation();
	const KShaderInformation* GetFSInformation();
	const KShaderInformation* GetMSInformation();

	IKShaderPtr GetVSShader(const VertexFormat* formats, size_t count);
	IKShaderPtr GetVSInstanceShader(const VertexFormat* formats, size_t count);
	// TODO 更换成Semantic
	IKShaderPtr GetMSShader(const VertexFormat* formats, size_t count);
	IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding);
};