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
	bool SetTextrue(uint8_t slot, const std::string& path) override;
	IKTexturePtr GetTexture(uint8_t slot) override;
	IKSamplerPtr GetSampler(uint8_t slot) override;
	bool Clear() override;
};