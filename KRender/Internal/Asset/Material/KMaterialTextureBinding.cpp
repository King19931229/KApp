#include "KMaterialTextureBinding.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"

KMaterialTextureBinding::KMaterialTextureBinding()
{
}

KMaterialTextureBinding::~KMaterialTextureBinding()
{
	for (size_t i = 0; i < ARRAY_SIZE(m_Textures); ++i)
	{
		ASSERT_RESULT(!m_Textures[i]);
	}
	for (size_t i = 0; i < ARRAY_SIZE(m_VirtualTextures); ++i)
	{
		ASSERT_RESULT(!m_VirtualTextures[i]);
	}
	for (size_t i = 0; i < ARRAY_SIZE(m_Samplers); ++i)
	{
		ASSERT_RESULT(!m_Samplers[i]);
	}
}

KSamplerDescription KMaterialTextureBinding::ToSamplerDesc(const KMeshTextureSampler& sampler)
{
	KSamplerDescription desc;

	auto ToFilterMode = [](MeshTextureFilter filter)
	{
		switch (filter)
		{
			case MTF_LINEAR:
				return FM_LINEAR;
			case MTF_CLOSEST:
				return FM_NEAREST;
			default:
				assert(false && "should not reach");
				return FM_NEAREST;
		}
	};

	auto ToAddressMode = [](MeshTextureAddress address)
	{
		switch (address)
		{
			case MTA_REPEAT:
				return AM_REPEAT;
			case MTA_CLAMP_TO_EDGE:
				return AM_CLAMP_TO_EDGE;
			case MTA_MIRRORED_REPEAT:
				return AM_MIRROR_CLAMP_TO_EDGE;
			default:
				assert(false && "should not reach");
				return AM_REPEAT;
		}
	};

	desc.minFilter = ToFilterMode(sampler.minFilter);
	desc.magFilter = ToFilterMode(sampler.magFilter);
	desc.addressU = ToAddressMode(sampler.addressModeU);
	desc.addressV = ToAddressMode(sampler.addressModeV);
	desc.addressW = ToAddressMode(sampler.addressModeW);

	return desc;
}

bool KMaterialTextureBinding::SetTextureVirtual(uint8_t slot, const std::string& path, uint32_t tileNum, const KMeshTextureSampler& sampler)
{
	if (slot < GetNumSlot())
	{
		UnsetTextrue(slot);
		if (!path.empty())
		{
			if (KRenderGlobal::TextureManager.Acquire(path.c_str(), tileNum, m_VirtualTextures[slot], false))
			{
				KSamplerDescription desc = ToSamplerDesc(sampler);
				desc.minMipmap = 0;
				desc.maxMipmap = m_VirtualTextures[slot]->GetMaxMipLevel();
				desc.anisotropic = false;
				desc.minFilter = FM_NEAREST;
				desc.magFilter = FM_NEAREST;

				ASSERT_RESULT(KRenderGlobal::SamplerManager.Acquire(desc, m_Samplers[slot]));
				return true;
			}
			else
			{
				KLog::Logger->Log(LL_ERROR, "Couldn't load texture file %s", path.c_str());
				return false;
			}
		}
	}
	return false;
}

bool KMaterialTextureBinding::SetTexture(uint8_t slot, const std::string& path, const KMeshTextureSampler& sampler)
{
	if (slot < GetNumSlot())
	{
		UnsetTextrue(slot);
		if (!path.empty())
		{
			if (KRenderGlobal::TextureManager.Acquire(path.c_str(), m_Textures[slot], false))
			{
				KSamplerDescription desc = ToSamplerDesc(sampler);
				desc.minMipmap = 0;
				desc.maxMipmap = (*m_Textures[slot])->GetMipmaps() - 1;
				desc.anisotropic = true;
				desc.anisotropicCount = 16;

				ASSERT_RESULT(KRenderGlobal::SamplerManager.Acquire(desc, m_Samplers[slot]));
				return true;
			}
			else
			{
				KLog::Logger->Log(LL_ERROR, "Couldn't load texture file %s", path.c_str());
				return false;
			}
		}
	}
	return false;
}

bool KMaterialTextureBinding::SetTexture(uint8_t slot, const std::string& name, const KCodecResult& result, const KMeshTextureSampler& sampler)
{
	if (slot < GetNumSlot())
	{
		UnsetTextrue(slot);
		if (result.pData)
		{
			if (KRenderGlobal::TextureManager.Acquire(
				name.c_str(),
				result.pData->GetData(),
				result.pData->GetSize(),
				result.uWidth, result.uHeight, result.uDepth,
				result.eFormat, result.bCubemap, true, m_Textures[slot], false))
			{
				KSamplerDescription desc = ToSamplerDesc(sampler);
				desc.minMipmap = 0;
				desc.maxMipmap = (*m_Textures[slot])->GetMipmaps() - 1;
			
				ASSERT_RESULT(KRenderGlobal::SamplerManager.Acquire(desc, m_Samplers[slot]));
				return true;
			}
			else
			{
				KLog::Logger->Log(LL_ERROR, "Empty texture data");
				return false;
			}
		}
	}
	return false;
}

bool KMaterialTextureBinding::SetErrorTexture(uint8_t slot)
{
	if (slot < GetNumSlot())
	{
		UnsetTextrue(slot);
		KRenderGlobal::TextureManager.GetErrorTexture(m_Textures[slot]);
		KRenderGlobal::SamplerManager.GetErrorSampler(m_Samplers[slot]);
		KLog::Logger->Log(LL_WARNING, "Assign an error texture to slot [%d]", slot);
		return true;
	}
	return false;
}

bool KMaterialTextureBinding::UnsetTextrue(uint8_t slot)
{
	if (slot < GetNumSlot())
	{
		m_Textures[slot].Release();
		m_VirtualTextures[slot].Release();
		m_Samplers[slot].Release();
		return true;
	}
	return false;
}

void* KMaterialTextureBinding::GetVirtualTextureSoul(uint8_t slot) const
{
	if (slot < GetNumSlot())
	{
		return m_VirtualTextures[slot].Get();
	}
	return nullptr;
}

IKTexturePtr KMaterialTextureBinding::GetTexture(uint8_t slot) const
{
	if (slot < GetNumSlot())
	{
		if (m_Textures[slot])
		{
			return *m_Textures[slot];
		}
		else if (m_VirtualTextures[slot])
		{
			return m_VirtualTextures[slot]->GetTableTexture();
		}
	}
	return nullptr;
}

IKSamplerPtr KMaterialTextureBinding::GetSampler(uint8_t slot) const
{
	if (slot < GetNumSlot())
	{
		return *m_Samplers[slot];
	}
	return nullptr;
}

bool KMaterialTextureBinding::GetIsVirtualTexture(uint8_t slot) const
{
	if (slot < GetNumSlot())
	{
		return m_VirtualTextures[slot].Get() != nullptr;
	}
	return false;
}

bool KMaterialTextureBinding::Duplicate(IKMaterialTextureBindingPtr& parameter)
{
	parameter = IKMaterialTextureBindingPtr(KNEW KMaterialTextureBinding());
	for (size_t i = 0; i < ARRAY_SIZE(m_Textures); ++i)
	{
		if (m_Textures[i] && m_Samplers[i])
		{
			// TODO
			parameter->SetTexture((uint8_t)i, (*m_Textures[i])->GetPath(), KMeshTextureSampler());
		}
	}
	return true;
}

bool KMaterialTextureBinding::Paste(const IKMaterialTextureBindingPtr& parameter)
{
	if (parameter)
	{
		Clear();

		KMaterialTextureBinding* source = (KMaterialTextureBinding*)parameter.get();
		for (size_t i = 0; i < ARRAY_SIZE(source->m_Textures); ++i)
		{
			if (source->m_Textures[i] && source->m_Samplers[i])
			{
				// TODO
				SetTexture((uint8_t)i, (*source->m_Textures[i])->GetPath(), KMeshTextureSampler());
			}
		}

		return true;
	}
	return false;
}

bool KMaterialTextureBinding::Clear()
{
	for (size_t i = 0; i < ARRAY_SIZE(m_Textures); ++i)
	{
		m_Textures[i].Release();
	}

	for (size_t i = 0; i < ARRAY_SIZE(m_VirtualTextures); ++i)
	{
		m_VirtualTextures[i].Release();
	}

	for (size_t i = 0; i < ARRAY_SIZE(m_Samplers); ++i)
	{
		m_Samplers[i].Release();
	}

	return true;
}