#pragma once
#include "Interface/IKShader.h"
#include "Interface/IKRenderCommand.h"
#include "Interface/IKMaterial.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>

enum MacroType
{
	MT_HAS_VERTEX_INPUT,
	MT_HAS_TEXTURE,
	MT_HAS_VIRTUAL_TEXTURE
};

struct KShaderMapMacro
{
	const char* macro;
	VertexFormat vertexFormat;
	uint32_t textureIndex;
	MacroType type;
};

static constexpr KShaderMapMacro PERMUTATING_MACRO[]
{
	// { macro, vertexFormat, textureIndex, type }
	{ "TANGENT_BINORMAL_INPUT",				VF_TANGENT_BINORMAL,		-1,		MT_HAS_VERTEX_INPUT },
	{ "BLEND_WEIGHT_INPUT",					VF_BLEND_WEIGHTS_INDICES,	-1,		MT_HAS_VERTEX_INPUT },
	{ "UV2_INPUT",							VF_UV2,						-1,		MT_HAS_VERTEX_INPUT },

	{ "VERTEX_COLOR_INPUT0",				VF_COLOR0,					-1,		MT_HAS_VERTEX_INPUT },
	{ "VERTEX_COLOR_INPUT1",				VF_COLOR1,					-1,		MT_HAS_VERTEX_INPUT },
	{ "VERTEX_COLOR_INPUT2",				VF_COLOR2,					-1,		MT_HAS_VERTEX_INPUT },
	{ "VERTEX_COLOR_INPUT3",				VF_COLOR3,					-1,		MT_HAS_VERTEX_INPUT },
	{ "VERTEX_COLOR_INPUT4",				VF_COLOR4,					-1,		MT_HAS_VERTEX_INPUT },
	{ "VERTEX_COLOR_INPUT5",				VF_COLOR5,					-1,		MT_HAS_VERTEX_INPUT },

	{ "HAS_MATERIAL_TEXTURE0",				VF_UNKNOWN,					0,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE1",				VF_UNKNOWN,					1,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE2",				VF_UNKNOWN,					2,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE3",				VF_UNKNOWN,					3,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE4",				VF_UNKNOWN,					4,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE5",				VF_UNKNOWN,					5,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE6",				VF_UNKNOWN,					6,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE7",				VF_UNKNOWN,					7,		MT_HAS_TEXTURE },

	{ "HAS_MATERIAL_TEXTURE8",				VF_UNKNOWN,					8,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE9",				VF_UNKNOWN,					9,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE10",				VF_UNKNOWN,					10,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE11",				VF_UNKNOWN,					11,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE12",				VF_UNKNOWN,					12,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE13",				VF_UNKNOWN,					13,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE14",				VF_UNKNOWN,					14,		MT_HAS_TEXTURE },
	{ "HAS_MATERIAL_TEXTURE15",				VF_UNKNOWN,					15,		MT_HAS_TEXTURE },

	{ "HAS_VIRTUAL_MATERIAL_TEXTURE0",		VF_UNKNOWN,					0,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE1",		VF_UNKNOWN,					1,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE2",		VF_UNKNOWN,					2,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE3",		VF_UNKNOWN,					3,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE4",		VF_UNKNOWN,					4,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE5",		VF_UNKNOWN,					5,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE6",		VF_UNKNOWN,					6,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE7",		VF_UNKNOWN,					7,		MT_HAS_VIRTUAL_TEXTURE },

	{ "HAS_VIRTUAL_MATERIAL_TEXTURE8",		VF_UNKNOWN,					8,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE9",		VF_UNKNOWN,					9,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE10",		VF_UNKNOWN,					10,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE11",		VF_UNKNOWN,					11,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE12",		VF_UNKNOWN,					12,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE13",		VF_UNKNOWN,					13,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE14",		VF_UNKNOWN,					14,		MT_HAS_VIRTUAL_TEXTURE },
	{ "HAS_VIRTUAL_MATERIAL_TEXTURE15",		VF_UNKNOWN,					15,		MT_HAS_VIRTUAL_TEXTURE },
};

static constexpr size_t MACRO_SIZE = sizeof(PERMUTATING_MACRO) / sizeof(KShaderMapMacro);

extern uint32_t VERTEX_FORMAT_MACRO_INDEX[VF_COUNT];
extern uint32_t MATERIAL_TEXTURE_BINDING_MACRO_INDEX[MAX_MATERIAL_TEXTURE_BINDING];

class KTextureBinding
{
protected:
	IKTexturePtr m_Textures[MAX_MATERIAL_TEXTURE_BINDING];
	bool m_IsVirtual[MAX_MATERIAL_TEXTURE_BINDING];
public:
	KTextureBinding()
	{
		memset(m_IsVirtual, 0, sizeof(m_IsVirtual));
	}

	~KTextureBinding()
	{
		Empty();
	}

	IKTexturePtr GetTexture(uint32_t slot) const
	{
		assert(slot < MAX_MATERIAL_TEXTURE_BINDING);
		return m_Textures[slot];
	}

	bool IsVirtual(uint32_t slot) const
	{
		assert(slot < MAX_MATERIAL_TEXTURE_BINDING);
		return m_IsVirtual[slot];
	}

	void AssignTexture(uint32_t slot, IKTexturePtr texture)
	{
		assert(slot < MAX_MATERIAL_TEXTURE_BINDING);
		m_Textures[slot] = texture;
	}

	void SetIsVirtual(uint32_t slot, bool isVirtual)
	{
		assert(slot < MAX_MATERIAL_TEXTURE_BINDING);
		m_IsVirtual[slot] = isVirtual;
	}

	void Empty()
	{
		for (size_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
		{
			m_Textures[i] = nullptr;
			m_IsVirtual[i] = false;
		}
	}
};

struct KShaderMapInitContext
{
	std::string vsFile;
	std::string fsFile;
	std::vector<std::tuple<std::string, std::string>> includeSources;
	std::vector<std::tuple<std::string, std::function<std::string()>>> includeFiles;
	std::vector<std::tuple<std::string, std::string>> macros;
};

class KShaderMap
{
protected:
	typedef std::unordered_map<size_t, KShaderRef> ShaderMap;
	ShaderMap m_VSShaderMap;
	ShaderMap m_VSInstanceShaderMap;
	ShaderMap m_VSGPUSceneShaderMap;
	ShaderMap m_FSShaderMap;
	ShaderMap m_FSGPUSceneShaderMap;

	IKShaderPtr m_VSTemplateShader;
	IKShaderPtr m_FSTemplateShader;

	std::string m_VSFile;
	std::string m_FSFile;
	std::vector<IKShader::IncludeSource> m_IncludeSources;
	std::vector<IKShader::IncludeFile> m_IncludeFiles;
	std::vector<IKShader::MacroPair> m_Macros;

	bool m_Async;

	typedef std::vector<IKShader::MacroPair> Macros;
	typedef std::shared_ptr<Macros> MacrosPtr;
	typedef std::unordered_map<size_t, MacrosPtr> MacrosMap;
	typedef std::unordered_set<size_t> MacrosSet;

	static std::mutex STATIC_RESOURCE_LOCK;
	static MacrosMap VS_MACROS_MAP;
	static MacrosMap VS_INSTANCE_MACROS_MAP;
	static MacrosMap VS_GPUSCENE_MACROS_MAP;
	static MacrosMap FS_MACROS_MAP;
	static MacrosMap FS_GPUSCENE_MACROS_MAP;
	static MacrosSet MACROS_SET;

	static size_t GenHash(const bool* macrosToEnable);
	static size_t CalcHash(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding);

	static void EnsureMacroMap(const bool* macrosToEnable);
	static void PermutateMacro(bool* macrosToEnable, size_t permutateIndex);

	IKShaderPtr GetShaderImplement(ShaderType shaderType, ShaderMap& shaderMap, MacrosMap& marcoMap, const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding);
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
	bool IsPermutationLoaded(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding);

	const std::string& GetVSPath() const { return m_VSFile; }
	const std::string& GetFSPath() const { return m_FSFile; }

	const KShaderInformation* GetVSInformation();
	const KShaderInformation* GetFSInformation();

	IKShaderPtr GetVSShader(const VertexFormat* formats, size_t count);
	IKShaderPtr GetVSInstanceShader(const VertexFormat* formats, size_t count);
	IKShaderPtr GetVSGPUSceneShader(const VertexFormat* formats, size_t count);
	IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding);
	IKShaderPtr GetFSGPUSceneShader(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding);
};