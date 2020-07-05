#pragma once
#include "KRender/Interface/IKShader.h"
#include "KRender/Interface/IKTexture.h"
#include "KRender/Interface/IKPipeline.h"
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

	virtual uint8_t GetNumSlot() const = 0;

	virtual bool SetTextrue(uint8_t slot, const std::string& path) = 0;

	virtual IKTexturePtr GetTexture(uint8_t slot) = 0;
	virtual IKSamplerPtr GetSampler(uint8_t slot) = 0;

	virtual bool Clear() = 0;
};

enum MaterialBlendMode
{
	OPAQUE,
	TRANSRPANT
};

struct IKMaterial;
typedef std::shared_ptr<IKMaterial> IKMaterialPtr;

struct IKMaterial
{
	virtual ~IKMaterial() {}

	virtual const IKMaterialParameterPtr GetVSParameter() = 0;
	virtual const IKMaterialParameterPtr GetFSParameter() = 0;

	virtual const IKMaterialTextureBindingPtr GetDefaultMaterialTexture() = 0;

	virtual const KShaderInformation::Constant* GetVSShadingInfo() = 0;
	virtual const KShaderInformation::Constant* GetFSShadingInfo() = 0;

	virtual const IKShaderPtr GetVSShader() = 0;
	virtual const IKShaderPtr GetVSInstanceShader() = 0;
	virtual const IKShaderPtr GetFSShader() = 0;

	virtual MaterialBlendMode GetBlendMode() const = 0;
	virtual void SetBlendMode(MaterialBlendMode mode) = 0;

	virtual IKPipelinePtr CreatePipeline(size_t frameIndex, const VertexFormat* formats, size_t count) = 0;
	virtual IKPipelinePtr CreateInstancePipeline(size_t frameIndex, const VertexFormat* formats, size_t count) = 0;

	virtual bool InitFromFile(const std::string& path, bool async) = 0;
	virtual bool Init(const std::string& vs, const std::string& fs, bool async) = 0;
	virtual bool UnInit() = 0;

	virtual const std::string& GetPath() const = 0;
	virtual bool SaveAsFile(const std::string& path) = 0;

	virtual bool Reload() = 0;
};