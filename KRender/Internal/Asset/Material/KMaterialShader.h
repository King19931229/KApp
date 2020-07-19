#pragma once
#include "Interface/IKShader.h"
#include "Interface/IKRenderCommand.h"
#include <unordered_map>

class KMaterialShader
{
protected:
	typedef std::unordered_map<size_t, IKShaderPtr> ShaderMap;
	ShaderMap m_VSShaderMap;
	ShaderMap m_VSInstanceShaderMap;
	ShaderMap m_FSShaderMap;

	IKShaderPtr m_VSTemplateShader;
	IKShaderPtr m_FSTemplateShader;

	std::string m_VSFile;
	std::string m_FSFile;

	size_t GenHash(const bool* macrosToEnable, size_t macrosSize);
	size_t CalcHash(const VertexFormat* formats, size_t count);
	void PermutateShader(const char** marcosToPermutate,
		bool* macrosToEnable,
		size_t macrosSize,
		size_t permutateIndex,
		bool async
	);
public:
	KMaterialShader();
	~KMaterialShader();

	bool Init(const std::string& vsFile, const std::string& fsFile, bool async);
	bool UnInit();
	bool Reload();

	bool IsInit();

	bool IsVSLoaded();
	bool IsFSLoaded();

	const std::string& GetVSPath() const { return m_VSFile; }
	const std::string& GetFSPath() const { return m_FSFile; }

	const KShaderInformation* GetVSInformation();
	const KShaderInformation* GetFSInformation();

	IKShaderPtr GetVSShader(const VertexFormat* formats, size_t count);
	IKShaderPtr GetVSInstanceShader(const VertexFormat* formats, size_t count);
	IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count);
};