#pragma once
#include "Interface/IKMaterial.h"
#include "KBase/Interface/IKXML.h"
#include "Material/KMaterialShader.h"

class KMaterial : public IKMaterial
{
protected:
	std::string m_Path;
	MaterialBlendMode m_BlendMode;
	KMaterialShader m_Shader;
	
	IKMaterialParameterPtr m_VSParameter;
	IKMaterialParameterPtr m_FSParameter;
	IKMaterialTextureBindingPtr m_MaterialTexture;
	KShaderInformation::Constant m_VSConstantInfo;
	KShaderInformation::Constant m_FSConstantInfo;
	bool m_VSInfoCalced;
	bool m_FSInfoCalced;
	bool m_VSParameterVerified;
	bool m_FSParameterVerified;

	bool m_ParameterReload;

	static const char* msMaterialRootKey;

	static const char* msVSKey;
	static const char* msMSKey;
	static const char* msFSKey;

	static const char* msMaterialTextureBindingKey;
	static const char* msMaterialTextureSlotKey;
	static const char* msMaterialTextureSlotIndexKey;
	static const char* msMaterialTextureSlotPathKey;

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
	bool CreateParameter(const KShaderInformation& information, IKMaterialParameterPtr& parameter);

	void BindSampler(IKPipelinePtr pipeline, const KShaderInformation& info);
	IKPipelinePtr CreatePipelineImpl(size_t frameIndex, const VertexFormat* formats, size_t count, IKShaderPtr vertexShader, IKShaderPtr fragmentShader);
	IKPipelinePtr CreateMeshPipelineImpl(size_t frameIndex, const VertexFormat* formats, size_t count, IKShaderPtr meshShader, IKShaderPtr fragmentShader);

	bool SaveParameterElement(const IKMaterialParameterPtr parameter, IKXMLElementPtr element) const;
	bool ReadParameterElement(IKMaterialParameterPtr parameter, const IKXMLElementPtr element, bool createNewParameter);

	bool SaveMaterialTexture(const IKMaterialTextureBindingPtr materialTexture, IKXMLElementPtr element) const;
	bool ReadMaterialTexture(IKMaterialTextureBindingPtr parameter, const IKXMLElementPtr elemment);

	bool ReadXMLContent(std::vector<char>& content);
public:
	KMaterial();
	~KMaterial();

	virtual IKShaderPtr GetVSShader(const VertexFormat* formats, size_t count);
	virtual IKShaderPtr GetVSInstanceShader(const VertexFormat* formats, size_t count);
	virtual IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding);
	virtual IKShaderPtr GetMSShader(const VertexFormat* formats, size_t count);

	virtual bool HasMSShader() const;
	virtual bool IsShaderLoaded(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding);

	virtual const IKMaterialParameterPtr GetVSParameter();
	virtual const IKMaterialParameterPtr GetFSParameter();

	virtual const IKMaterialTextureBindingPtr GetDefaultMaterialTexture();

	virtual const KShaderInformation::Constant* GetVSShadingInfo();
	virtual const KShaderInformation::Constant* GetFSShadingInfo();

	virtual MaterialBlendMode GetBlendMode() const { return m_BlendMode; }
	virtual void SetBlendMode(MaterialBlendMode mode) { m_BlendMode = mode; }

	virtual IKPipelinePtr CreatePipeline(size_t frameIndex, const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding);
	virtual IKPipelinePtr CreateMeshPipeline(size_t frameIndex, const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding);
	virtual IKPipelinePtr CreateInstancePipeline(size_t frameIndex, const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding);

	virtual bool InitFromFile(const std::string& path, bool async);
	virtual bool Init(const std::string& vs, const std::string& fs, const std::string& ms, bool async);
	virtual bool UnInit();

	virtual const std::string& GetPath() const { return m_Path; }

	virtual bool SaveAsFile(const std::string& path);

	virtual bool Reload();
};