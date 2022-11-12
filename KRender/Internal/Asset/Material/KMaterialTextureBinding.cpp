#include "KMaterialTextureBinding.h"
#include "Internal/KRenderGlobal.h"
#include <assert.h>

KMaterialTextureBinding::KMaterialTextureBinding()
{
}

KMaterialTextureBinding::~KMaterialTextureBinding()
{
	for (size_t i = 0; i < ARRAY_SIZE(m_Textures); ++i)
	{
		ASSERT_RESULT(!m_Textures[i]);
	}
	for (size_t i = 0; i < ARRAY_SIZE(m_Samplers); ++i)
	{
		ASSERT_RESULT(!m_Samplers[i]);
	}
}

uint8_t KMaterialTextureBinding::GetNumSlot() const
{
	return ARRAY_SIZE(m_Textures);
}

bool KMaterialTextureBinding::SetTexture(uint8_t slot, const std::string& path)
{
	if (!path.empty() && slot < GetNumSlot())
	{
		UnsetTextrue(slot);

		if (KRenderGlobal::TextureManager.Acquire(path.c_str(), m_Textures[slot], false))
		{
			KSamplerDescription desc;
			desc.minFilter = desc.magFilter = FM_LINEAR;
			desc.minMipmap = 0;
			desc.maxMipmap = (*m_Textures[slot])->GetMipmaps() - 1;
			ASSERT_RESULT(KRenderGlobal::SamplerManager.Acquire(desc, m_Samplers[slot]));
			return true;
		}
	}
	return false;
}

bool KMaterialTextureBinding::UnsetTextrue(uint8_t slot)
{
	if (slot < GetNumSlot())
	{
		m_Textures[slot].Release();
		m_Samplers[slot].Release();
		return true;
	}
	return false;
}

IKTexturePtr KMaterialTextureBinding::GetTexture(uint8_t slot) const
{
	if (slot < GetNumSlot())
	{
		return *m_Textures[slot];
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

bool KMaterialTextureBinding::Duplicate(IKMaterialTextureBindingPtr& parameter)
{
	parameter = IKMaterialTextureBindingPtr(KNEW KMaterialTextureBinding());
	for (size_t i = 0; i < ARRAY_SIZE(m_Textures); ++i)
	{
		if (m_Textures[i] && m_Samplers[i])
		{
			parameter->SetTexture((uint8_t)i, (*m_Textures[i])->GetPath());
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
				SetTexture((uint8_t)i, (*source->m_Textures[i])->GetPath());
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

	for (size_t i = 0; i < ARRAY_SIZE(m_Samplers); ++i)
	{
		m_Samplers[i].Release();
	}

	return true;
}