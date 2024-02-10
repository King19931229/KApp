#pragma once
#include "Interface/IKMaterial.h"
#include "KBase/Interface/IKXML.h"
#include "Internal/ShaderMap/KShaderMap.h"

class KMaterial : public IKMaterial
{
protected:
	uint32_t m_Version;
	std::string m_Path;
	std::string m_ShaderFile;
	std::string m_MaterialCode;
	MaterialShadingMode m_ShadingMode;
	IKMaterialParameterPtr m_Parameter;
	IKMaterialTextureBindingPtr m_TextureBinding;
	KShaderInformation::Constant m_ConstantInfo;
	bool m_DoubleSide;

	bool m_InfoCalced;
	bool m_ParameterVerified;
	bool m_ParameterNeedRebuild;

	KShaderMap m_ShaderMap;
	KShaderMap m_StaticCSMShaderMap;
	KShaderMap m_DynamicCSMShaderMap;

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

	bool SaveParameterElement(const IKMaterialParameterPtr parameter, IKXMLElementPtr element) const;
	bool ReadParameterElement(IKMaterialParameterPtr parameter, const IKXMLElementPtr element, bool createNewParameter);

	bool SaveMaterialTexture(const IKMaterialTextureBindingPtr materialTexture, IKXMLElementPtr element) const;
	bool ReadMaterialTexture(IKMaterialTextureBindingPtr parameter, const IKXMLElementPtr elemment);

	bool ReadXMLContent(std::vector<char>& content);

	struct PipelineCreateContext
	{
		bool depthBiasEnable;
	};

	IKPipelinePtr CreatePipelineInternal(const PipelineCreateContext& context, const VertexFormat* formats, size_t count, IKShaderPtr vertexShader, IKShaderPtr fragmentShader);

	IKPipelinePtr CreatePipelineImpl(KShaderMap& shaderMap, const PipelineCreateContext& context, const VertexFormat* formats, size_t count);
	IKPipelinePtr CreateInstancePipelineImpl(KShaderMap& shaderMap, const PipelineCreateContext& context, const VertexFormat* formats, size_t count);
public:
	KMaterial();
	~KMaterial();

	virtual IKShaderPtr GetVSShader(const VertexFormat* formats, size_t count);
	virtual IKShaderPtr GetVSInstanceShader(const VertexFormat* formats, size_t count);
	virtual IKShaderPtr GetVSGPUSceneShader(const VertexFormat* formats, size_t count);
	virtual IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count);
	virtual IKShaderPtr GetFSGPUSceneShader(const VertexFormat* formats, size_t count);

	virtual bool IsShaderLoaded(const VertexFormat* formats, size_t count);

	virtual const IKMaterialParameterPtr GetParameter();

	virtual const IKMaterialTextureBindingPtr GetTextureBinding();

	virtual const KShaderInformation::Constant* GetShadingInfo();

	virtual const KShaderInformation* GetVSInformation();
	virtual const KShaderInformation* GetFSInformation();

	virtual MaterialShadingMode GetShadingMode() const { return m_ShadingMode; }
	virtual void SetShadingMode(MaterialShadingMode mode) { m_ShadingMode = mode; }

	virtual IKPipelinePtr CreatePipeline(const VertexFormat* formats, size_t count);
	virtual IKPipelinePtr CreateInstancePipeline(const VertexFormat* formats, size_t count);

	virtual IKPipelinePtr CreateCSMPipeline(const VertexFormat* formats, size_t count, bool staticCSM);
	virtual IKPipelinePtr CreateCSMInstancePipeline(const VertexFormat* formats, size_t count, bool staticCSM);

	virtual bool InitFromFile(const std::string& path, bool async);
	virtual bool InitFromImportAssetMaterial(const KMeshRawData::Material& input, bool async);
	virtual bool UnInit();

	virtual const std::string& GetMaterialGeneratedCode() const { return m_MaterialCode; }
	virtual const std::string& GetPath() const { return m_Path; }

	virtual bool SaveAsFile(const std::string& path);

	virtual bool IsDoubleSide() const { return m_DoubleSide; }
	virtual void SetDoubleSide(bool doubleSide) { m_DoubleSide = doubleSide; }

	virtual bool Reload();
};