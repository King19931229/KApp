#pragma once
#include "Interface/IKMaterial.h"
#include "KBase/Interface/IKXML.h"

class KMaterial : public IKMaterial
{
protected:
	std::string m_Path;

	IKShaderPtr m_VSShader;
	IKShaderPtr m_VSInstanceShader;
	IKShaderPtr m_FSShader;
	IKPipelinePtr m_Pipeline;
	IKPipelinePtr m_InstancePipeline;
	IKMaterialParameterPtr m_VSParameter;
	IKMaterialParameterPtr m_FSParameter;
	MaterialBlendMode m_BlendMode;
	bool m_VSParameterVerified;
	bool m_FSParameterVerified;

	static const char* msVSKey;
	static const char* msFSKey;
	static const char* msBlendModeKey;
	static const char* msVSParameterKey;
	static const char* msFSParameterKey;
	static const char* msParameterValueKey;
	static const char* msParameterValueNameKey;
	static const char* msParameterValueTypeKey;
	static const char* msParameterValueVecSizeKey;

	static MaterialValueType ShaderConstantTypeToMaterialType(KShaderInformation::Constant::ConstantMemberType type);
	static MaterialValueType StringToMaterialValueType(const char* type);
	static const char* MaterialValueTypeToString(MaterialValueType type);
	static MaterialBlendMode StringToMaterialBlendMode(const char* mode);
	static const char* MaterialBlendModeToString(MaterialBlendMode mode);

	bool VerifyParameter(IKMaterialParameterPtr parameter, const KShaderInformation& information);
	IKMaterialParameterPtr CreateParameter(const KShaderInformation& information);

	IKPipelinePtr CreatePipelineImpl(size_t frameIndex, const VertexFormat* formats, size_t count, IKShaderPtr vertexShader, IKShaderPtr fragmentShader);

	bool SaveParameterElement(const IKMaterialParameterPtr parameter, IKXMLElementPtr elemment) const;
	bool ReadParameterElement(IKMaterialParameterPtr parameter, const IKXMLElementPtr elemment);
public:
	KMaterial();
	~KMaterial();

	virtual const IKMaterialParameterPtr GetVSParameter();
	virtual const IKMaterialParameterPtr GetFSParameter();

	virtual const IKShaderPtr GetVSShader() { return m_VSShader; }
	virtual const IKShaderPtr GetVSInstanceShader() { return m_VSInstanceShader; }
	virtual const IKShaderPtr GetFSShader() { return m_FSShader; }

	virtual MaterialBlendMode GetBlendMode() const { return m_BlendMode; }
	virtual void SetBlendMode(MaterialBlendMode mode) { m_BlendMode = mode; }

	virtual IKPipelinePtr CreatePipeline(size_t frameIndex, const VertexFormat* formats, size_t count);
	virtual IKPipelinePtr CreateInstancePipeline(size_t frameIndex, const VertexFormat* formats, size_t count);

	virtual bool InitFromFile(const std::string& path, bool async);
	virtual bool Init(const std::string& vs, const std::string& fs, bool async);
	virtual bool UnInit();

	virtual const std::string& GetPath() const { return m_Path; }

	virtual bool SaveAsFile(const std::string& path);
};