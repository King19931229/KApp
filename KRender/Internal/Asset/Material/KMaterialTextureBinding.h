#pragma once
#include "Interface/IKMaterial.h"
#include "Internal/KRenderGlobal.h"

class KMaterialTextureBinding : public IKMaterialTextureBinding
{
protected:
	IKTexturePtr m_Textures[SHADER_BINDING_MATERIAL_COUNT];
	IKSamplerPtr m_Samplers[SHADER_BINDING_MATERIAL_COUNT];
public:
	KMaterialTextureBinding();
	virtual ~KMaterialTextureBinding();
	uint8_t GetNumSlot() const override;
	bool SetTexture(uint8_t slot, const std::string& path) override;
	IKTexturePtr GetTexture(uint8_t slot) const override;
	IKSamplerPtr GetSampler(uint8_t slot) const override;
	bool Duplicate(IKMaterialTextureBindingPtr& parameter) override;
	bool Paste(const IKMaterialTextureBindingPtr& parameter) override;
	bool Clear() override;
};