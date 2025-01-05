#pragma once
#include "KRender/Interface/IKShader.h"
#include "KRender/Interface/IKTexture.h"
#include "KRender/Interface/IKSampler.h"
#include "KRender/Interface/IKPipeline.h"
#include "KBase/Interface/IKAssetLoader.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class MaterialValueType
{
	BOOL,
	INT,
	FLOAT,
	UNKNOWN
};

constexpr uint32_t MAX_MATERIAL_TEXTURE_BINDING = 16;
constexpr uint32_t MAX_VIRTUAL_PHYSICAL_TEXTURE_BINDING = 4;
constexpr uint32_t MAX_USER_TEXTURE_BINDING = MAX_MATERIAL_TEXTURE_BINDING + MAX_VIRTUAL_PHYSICAL_TEXTURE_BINDING;

struct IKMaterialValue;
typedef std::shared_ptr<IKMaterialValue> IKMaterialValuePtr;

struct IKMaterialValue
{
	virtual ~IKMaterialValue() {}
	virtual const std::string& GetName() const = 0;
	virtual MaterialValueType GetType() const = 0;
	virtual uint8_t GetVecSize() const = 0;
	virtual const void* GetData() const = 0;
	virtual void SetData(const void* data) = 0;
};

struct IKMaterialParameter;
typedef std::shared_ptr<IKMaterialParameter> IKMaterialParameterPtr;

struct IKMaterialParameter
{
	virtual ~IKMaterialParameter() {}
	virtual bool HasValue(const std::string& name) const = 0;
	virtual IKMaterialValuePtr GetValue(const std::string& name) const = 0;
	virtual const std::vector<IKMaterialValuePtr>& GetAllValues() const = 0;
	virtual bool CreateValue(const std::string& name, MaterialValueType type, uint8_t vecSize, const void* initData = nullptr) = 0;
	virtual bool SetValue(const std::string& name, MaterialValueType type, uint8_t vecSize, const void* data) = 0;
	virtual bool RemoveValue(const std::string& name) = 0;
	virtual bool RemoveAllValues() = 0;

	virtual bool Duplicate(IKMaterialParameterPtr& parameter) = 0;
	virtual bool Paste(const IKMaterialParameterPtr& parameter) = 0;
};

struct IKMaterialTextureBinding;
typedef std::shared_ptr<IKMaterialTextureBinding> IKMaterialTextureBindingPtr;

struct IKMaterialTextureBinding
{
	virtual ~IKMaterialTextureBinding() {}

	static uint8_t GetNumSlot() { return MAX_MATERIAL_TEXTURE_BINDING;	}

	virtual bool SetTextureVirtual(uint8_t slot, const std::string& path, uint32_t tileNum, const KMeshTextureSampler& sampler) = 0;
	virtual bool SetTexture(uint8_t slot, const std::string& path, const KMeshTextureSampler& sampler) = 0;
	virtual bool SetTexture(uint8_t slot, const std::string& name, const KCodecResult& result, const KMeshTextureSampler& sampler) = 0;
	virtual bool SetErrorTexture(uint8_t slot) = 0;
	virtual bool UnsetTextrue(uint8_t slot) = 0;

	virtual IKTexturePtr GetTexture(uint8_t slot) const = 0;
	virtual IKSamplerPtr GetSampler(uint8_t slot) const = 0;
	virtual bool GetIsVirtualTexture(uint8_t slot) const = 0;
	virtual void* GetVirtualTextureSoul(uint8_t slot) const = 0;

	virtual bool Duplicate(IKMaterialTextureBindingPtr& parameter) = 0;
	virtual bool Paste(const IKMaterialTextureBindingPtr& parameter) = 0;

	virtual bool IsResourceReady() const = 0;

	virtual bool Clear() = 0;
};

enum MaterialShadingMode
{
	MSM_OPAQUE,
	MSM_TRANSRPANT
};

struct IKMaterial;
typedef std::shared_ptr<IKMaterial> IKMaterialPtr;

struct IKMaterial
{
	virtual ~IKMaterial() {}

	virtual IKShaderPtr GetVSShader(const VertexFormat* formats, size_t count) = 0;
	virtual IKShaderPtr GetVSInstanceShader(const VertexFormat* formats, size_t count) = 0;
	virtual IKShaderPtr GetVSGPUSceneShader(const VertexFormat* formats, size_t count) = 0;
	virtual IKShaderPtr GetFSShader(const VertexFormat* formats, size_t count) = 0;
	virtual IKShaderPtr GetFSGPUSceneShader(const VertexFormat* formats, size_t count) = 0;

	virtual IKShaderPtr GetVSVirtualFeedbackShader(const VertexFormat* formats, size_t count) = 0;
	virtual IKShaderPtr GetVSInstanceVirtualFeedbackShader(const VertexFormat* formats, size_t count) = 0;
	virtual IKShaderPtr GetFSVirtualFeedbackShader(const VertexFormat* formats, size_t count) = 0;

	virtual bool IsShaderLoaded(const VertexFormat* formats, size_t count) = 0;

	virtual const IKMaterialParameterPtr GetParameter() = 0;
	virtual const IKMaterialTextureBindingPtr GetTextureBinding() = 0;
	virtual const KShaderInformation::Constant* GetShadingInfo() = 0;

	virtual const KShaderInformation* GetVSInformation() = 0;
	virtual const KShaderInformation* GetFSInformation() = 0;

	virtual MaterialShadingMode GetShadingMode() const = 0;
	virtual void SetShadingMode(MaterialShadingMode mode) = 0;

	virtual IKPipelinePtr CreatePipeline(const VertexFormat* formats, size_t count) = 0;
	virtual IKPipelinePtr CreateInstancePipeline(const VertexFormat* formats, size_t coun) = 0;

	virtual IKPipelinePtr CreateCSMPipeline(const VertexFormat* formats, size_t count, bool staticCSM) = 0;
	virtual IKPipelinePtr CreateCSMInstancePipeline(const VertexFormat* formats, size_t count, bool staticCSM) = 0;

	virtual bool InitFromFile(const std::string& path, bool async) = 0;
	virtual bool InitFromImportAssetMaterial(const KMeshRawData::Material& input, bool async) = 0;
	virtual bool UnInit() = 0;

	virtual std::function<std::string()> GetMaterialGeneratedCodeReader() const = 0;

	virtual const std::string& GetPath() const = 0;
	virtual bool SaveAsFile(const std::string& path) = 0;

	virtual bool IsDoubleSide() const = 0;
	virtual void SetDoubleSide(bool doubleSide) = 0;

	virtual bool Reload() = 0;
};

typedef KReferenceHolder<IKMaterialPtr> KMaterialRef;