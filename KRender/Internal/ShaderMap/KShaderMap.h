#pragma once
#include "Interface/IKShader.h"
#include "Interface/IKRenderCommand.h"
#include "Interface/IKMaterial.h"
#include <unordered_map>
#include <mutex>

constexpr uint32_t MAX_SHADERMAP_TEXTURE_BINDING = 8;

class KTextureBinding
{
protected:
	IKTexturePtr m_TextureMap[MAX_SHADERMAP_TEXTURE_BINDING];
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
		assert(slot < MAX_SHADERMAP_TEXTURE_BINDING);
		return m_TextureMap[slot];
	}

	void AssignTexture(uint32_t slot, IKTexturePtr texture)
	{
		assert(slot < MAX_SHADERMAP_TEXTURE_BINDING);
		m_TextureMap[slot] = texture;
	}

	void Empty()
	{
		for (size_t i = 0; i < MAX_SHADERMAP_TEXTURE_BINDING; ++i)
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

	static std::mutex STATIC_RESOURCE_LOCK;
	static MacrosMap VS_MACROS_MAP;
	static MacrosMap VS_INSTANCE_MACROS_MAP;
	static MacrosMap MS_MACROS_MAP;
	static MacrosMap FS_MACROS_MAP;

	static size_t GenHash(const bool* macrosToEnable, size_t macrosSize);
	static size_t CalcHash(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding, bool meshletInput);

	static void PermutateMacro(const char** marcosToPermutate,
		bool* macrosToEnable,
		size_t macrosSize,
		size_t vsMacrosSize,
		size_t permutateIndex);

public:
	KShaderMap();
	~KShaderMap();

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
	IKShaderPtr GetMSShader(const VertexFormat* formats, size_t count);
	IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count, const KTextureBinding* textureBinding, bool meshletInput);
};