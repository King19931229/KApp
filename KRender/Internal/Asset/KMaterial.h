#pragma once
#include "Interface/IKMaterial.h"
#include "KBase/Interface/IKXML.h"

class KMaterial : public IKMaterial
{
protected:
	std::string m_Path;

	IKShaderPtr m_VSShader;
	IKShaderPtr m_FSShader;
	IKMaterialParameterPtr m_VSParameter;
	IKMaterialParameterPtr m_FSParameter;
	bool m_VSParameterVerified;
	bool m_FSParameterVerified;

	static const char* msVSKey;
	static const char* msFSKey;
	static const char* msVSParameterKey;
	static const char* msFSParameterKey;
	static const char* msParameterValueKey;
	static const char* msParameterValueNameKey;
	static const char* msParameterValueTypeKey;
	static const char* msParameterValueVecSizeKey;

	static MaterialValueType ShaderConstantTypeToMaterialType(KShaderInformation::Constant::ConstantMemberType type);
	static MaterialValueType StringToMaterialValueType(const char* type);
	static const char* MaterialValueTypeToString(MaterialValueType type);
	
	bool VerifyParameter(IKMaterialParameterPtr parameter, const KShaderInformation& information);
	IKMaterialParameterPtr CreateParameter(const KShaderInformation& information);

	bool SaveParameterElement(const IKMaterialParameterPtr parameter, IKXMLElementPtr elemment) const;
	bool ReadParameterElement(IKMaterialParameterPtr parameter, const IKXMLElementPtr elemment);
public:
	KMaterial();
	~KMaterial();

	virtual const IKMaterialParameterPtr GetVSParameter();
	virtual const IKMaterialParameterPtr GetFSParameter();

	virtual const IKShaderPtr GetVSShader() { return m_VSShader; }
	virtual const IKShaderPtr GetFSShader() { return m_FSShader; }

	virtual bool InitFromFile(const std::string& path, bool async);
	virtual bool Init(const std::string& vs, const std::string& fs, bool async);
	virtual bool UnInit();

	virtual const std::string& GetPath() const { return m_Path; }

	virtual bool SaveAsFile(const std::string& path);
};