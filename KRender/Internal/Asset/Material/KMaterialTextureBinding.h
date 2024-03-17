#pragma once
#include "Interface/IKMaterial.h"
#include "Interface/IKSampler.h"

class KMaterialTextureBinding : public IKMaterialTextureBinding
{
protected:
	KTextureRef m_Textures[MAX_MATERIAL_TEXTURE_BINDING];
	KSamplerRef m_Samplers[MAX_MATERIAL_TEXTURE_BINDING];
	bool m_IsVirtual[MAX_MATERIAL_TEXTURE_BINDING];
	KSamplerDescription ToSamplerDesc(const KMeshTextureSampler& sampler);
public:
	KMaterialTextureBinding();
	virtual ~KMaterialTextureBinding();

	// C++11定义析构函数将会压制移动(move)成员函数的自动生成
	KMaterialTextureBinding(KMaterialTextureBinding&& rhs) = default;
	KMaterialTextureBinding& operator=(KMaterialTextureBinding&& rhs) = default;
	// 类定义移动构造函数后 默认的复制构造函数会被删除
	// KMaterialTextureBinding(const KMaterialTextureBinding& rhs) = delete;
	// KMaterialTextureBinding& operator=(const KMaterialTextureBinding& rhs) = delete;

	bool SetTexture(uint8_t slot, const std::string& path, const KMeshTextureSampler& sampler) override;
	bool SetTexture(uint8_t slot, const std::string& name, const KCodecResult& result, const KMeshTextureSampler& sampler) override;
	bool SetErrorTexture(uint8_t slot) override;
	bool UnsetTextrue(uint8_t slot) override;
	bool SetIsVirtualTexture(uint8_t slot, bool isVirtual) override;
	IKTexturePtr GetTexture(uint8_t slot) const override;
	IKSamplerPtr GetSampler(uint8_t slot) const override;
	bool GetIsVirtualTexture(uint8_t slot) const override;
	bool Duplicate(IKMaterialTextureBindingPtr& parameter) override;
	bool Paste(const IKMaterialTextureBindingPtr& parameter) override;
	bool Clear() override;
};