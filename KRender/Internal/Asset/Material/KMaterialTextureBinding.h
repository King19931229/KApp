#pragma once
#include "Interface/IKMaterial.h"
#include "Interface/IKSampler.h"

class KMaterialTextureBinding : public IKMaterialTextureBinding
{
protected:
	KTextureRef m_Textures[SHADER_BINDING_MATERIAL_COUNT];
	KSamplerRef m_Samplers[SHADER_BINDING_MATERIAL_COUNT];
public:
	KMaterialTextureBinding();
	virtual ~KMaterialTextureBinding();

	// C++11定义析构函数将会压制移动(move)成员函数的自动生成
	KMaterialTextureBinding(KMaterialTextureBinding&& rhs) = default;
	KMaterialTextureBinding& operator=(KMaterialTextureBinding&& rhs) = default;
	// 类定义移动构造函数后 默认的复制构造函数会被删除
	// KMaterialTextureBinding(const KMaterialTextureBinding& rhs) = delete;
	// KMaterialTextureBinding& operator=(const KMaterialTextureBinding& rhs) = delete;

	uint8_t GetNumSlot() const override;
	bool SetTexture(uint8_t slot, const std::string& path) override;
	bool UnsetTextrue(uint8_t slot) override;
	IKTexturePtr GetTexture(uint8_t slot) const override;
	IKSamplerPtr GetSampler(uint8_t slot) const override;
	bool Duplicate(IKMaterialTextureBindingPtr& parameter) override;
	bool Paste(const IKMaterialTextureBindingPtr& parameter) override;
	bool Clear() override;
};