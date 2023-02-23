#pragma once
#include "Interface/IKMaterial.h"
#include "KBase/Interface/IKXML.h"
#include "Internal/ShaderMap/KShaderMap.h"

// 待重构
class KMaterial : public IKMaterial
{
protected:
	uint32_t m_Version;
	std::string m_Path;
	std::string m_ShaderFile;
	MaterialShadingMode m_ShadingMode;
	IKMaterialParameterPtr m_Parameter;
	IKMaterialTextureBindingPtr m_TextureBinding;
	KShaderInformation::Constant m_ConstantInfo;
	bool m_InfoCalced;
	bool m_ParameterVerified;
	bool m_ParameterNeedRebuild;

	KShaderMap m_ShaderMap;

	static uint32_t msCurrentVersion;

	static const char* msMaterialRootKey;

	static const char* msVersionKey;
	static const char* msShaderKey;

	static const char* msMaterialTextureBindingKey;
	static const char* msMaterialTextureSlotKey;
	static const char* msMaterialTextureSlotIndexKey;
	static const char* msMaterialTextureSlotPathKey;

	static const char* msShadingModeKey;

	static const char* msVSParameterKey;
	static const char* msFSParameterKey;
	static const char* msParameterValueKey;
	static const char* msParameterValueNameKey;
	static const char* msParameterValueTypeKey;
	static const char* msParameterValueVecSizeKey;

	static MaterialValueType ShaderConstantTypeToMaterialType(KShaderInformation::Constant::ConstantMemberType type);
	static MaterialValueType StringToMaterialValueType(const char* type);
	static const char* MaterialValueTypeToString(MaterialValueType type);
	static MaterialShadingMode StringToMaterialShadingMode(const char* mode);
	static const char* MaterialShadingModeToString(MaterialShadingMode mode);
	static KTextureBinding ConvertToTextureBinding(const IKMaterialTextureBinding* mtlTextureBinding);

	bool VerifyParameter(IKMaterialParameterPtr parameter, const KShaderInformation& information);
	bool CreateParameter(const KShaderInformation& information, IKMaterialParameterPtr& parameter);

	void BindSampler(IKPipelinePtr pipeline, const KShaderInformation& info);
	IKPipelinePtr CreatePipelineImpl(const VertexFormat* formats, size_t count, IKShaderPtr vertexShader, IKShaderPtr fragmentShader);
	IKPipelinePtr CreateMeshPipelineImpl(const VertexFormat* formats, size_t count, IKShaderPtr meshShader, IKShaderPtr fragmentShader);

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
	virtual IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count, bool meshletInput);
	virtual IKShaderPtr GetMSShader(const VertexFormat* formats, size_t count);

	virtual bool HasMSShader() const;
	virtual bool IsShaderLoaded(const VertexFormat* formats, size_t count);

	virtual const IKMaterialParameterPtr GetParameter();

	virtual const IKMaterialTextureBindingPtr GetTextureBinding();

	virtual const KShaderInformation::Constant* GetShadingInfo();

	virtual MaterialShadingMode GetShadingMode() const { return m_ShadingMode; }
	virtual void SetShadingMode(MaterialShadingMode mode) { m_ShadingMode = mode; }

	virtual IKPipelinePtr CreatePipeline(const VertexFormat* formats, size_t count);
	virtual IKPipelinePtr CreateMeshPipeline(const VertexFormat* formats, size_t count);
	virtual IKPipelinePtr CreateInstancePipeline(const VertexFormat* formats, size_t count);

	virtual bool InitFromFile(const std::string& path, bool async);
	virtual bool InitFromImportAssetMaterial(const KAssetImportResult::Material& input, bool async);
	virtual bool UnInit();

	virtual const std::string& GetPath() const { return m_Path; }

	virtual bool SaveAsFile(const std::string& path);

	virtual bool Reload();
};