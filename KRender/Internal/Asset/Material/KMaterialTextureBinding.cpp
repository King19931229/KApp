#include "KMaterialTextureBinding.h"
#include <assert.h>

KMaterialTextureBinding::KMaterialTextureBinding()
{
	for (size_t i = 0; i < ARRAY_SIZE(m_Textures); ++i)
	{
		m_Textures[i] = nullptr;
	}
	for (size_t i = 0; i < ARRAY_SIZE(m_Samplers); ++i)
	{
		m_Samplers[i] = nullptr;
	}
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
			ASSERT_RESULT(KRenderGlobal::TextureManager.CreateSampler(m_Samplers[slot]));
			m_Samplers[slot]->SetFilterMode(FM_LINEAR, FM_LINEAR);
			ASSERT_RESULT(m_Samplers[slot]->Init(m_Textures[slot], false));
			return true;
		}
	}
	return false;
}

bool KMaterialTextureBinding::UnsetTextrue(uint8_t slot)
{
	if (slot < GetNumSlot())
	{
		if (m_Textures[slot])
		{
			KRenderGlobal::TextureManager.Release(m_Textures[slot]);
			m_Textures[slot] = nullptr;
		}

		if (m_Samplers[slot])
		{
			KRenderGlobal::TextureManager.DestroySampler(m_Samplers[slot]);
			m_Samplers[slot] = nullptr;
		}
		return true;
	}
	return false;
}

IKTexturePtr KMaterialTextureBinding::GetTexture(uint8_t slot) const
{
	if (slot < GetNumSlot())
	{
		return m_Textures[slot];
	}
	return nullptr;
}

IKSamplerPtr KMaterialTextureBinding::GetSampler(uint8_t slot) const
{
	if (slot < GetNumSlot())
	{
		return m_Samplers[slot];
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
			parameter->SetTexture((uint8_t)i, m_Textures[i]->GetPath());
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
				SetTexture((uint8_t)i, source->m_Textures[i]->GetPath());
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
		if (m_Textures[i])
		{
			KRenderGlobal::TextureManager.Release(m_Textures[i]);
			m_Textures[i] = nullptr;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(m_Samplers); ++i)
	{
		if (m_Samplers[i])
		{
			KRenderGlobal::TextureManager.DestroySampler(m_Samplers[i]);
			m_Samplers[i] = nullptr;
		}
	}

	return true;
}