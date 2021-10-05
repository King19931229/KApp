#pragma once
#include "Interface/IKShader.h"
#include "Interface/IKRenderCommand.h"
#include "KMaterialTextureBinding.h"
#include <unordered_map>
#include <mutex>

class KMaterialShader
{
protected:
	typedef std::unordered_map<size_t, IKShaderPtr> ShaderMap;
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

	bool m_Async;

	typedef std::vector<IKShader::MacroPair> Macros;
	typedef std::shared_ptr<Macros> MacrosPtr;
	typedef std::unordered_map<size_t, MacrosPtr> MacrosMap;

	static std::mutex STATIC_RESOURCE_LOCK;
	static const char* PERMUTATING_MACRO[8];
	static MacrosMap VS_MACROS_MAP;
	static MacrosMap VS_INSTANCE_MACROS_MAP;
	static MacrosMap MS_MACROS_MAP;
	static MacrosMap FS_MACROS_MAP;

	static size_t GenHash(const bool* macrosToEnable, size_t macrosSize, size_t vsMacrosSize, bool vsOnly);
	static size_t CalcHash(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding);

	static void PermutateMacro(const char** marcosToPermutate,
		bool* macrosToEnable,
		size_t macrosSize,
		size_t vsMacrosSize,
		size_t permutateIndex);

public:
	KMaterialShader();
	~KMaterialShader();

	struct InitContext
	{
		std::string vsFile;
		std::string msFile;
		std::string fsFile;
	};

	bool Init(const InitContext& context, bool async);
	bool UnInit();
	bool Reload();

	bool IsInit();

	bool IsAllVSLoaded();
	bool IsAllFSLoaded();
	bool IsVSTemplateLoaded();
	bool IsFSTemplateLoaded();
	bool IsBothLoaded(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding);

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
	IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding);
};