#include "KVulkanSampler.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

KVulkanSampler::KVulkanSampler()
	: KSamplerBase(),
	m_SamplerInit(false)
{
	ZERO_MEMORY(m_TextureSampler);
}

KVulkanSampler::~KVulkanSampler()
{
	ASSERT_RESULT(!m_SamplerInit);
}

bool KVulkanSampler::Init()
{
	assert(KVulkanGlobal::deviceReady);
	assert(!m_SamplerInit);

	VkFilter magFilter = VK_FILTER_MAX_ENUM;
	VkFilter minFilter = VK_FILTER_MAX_ENUM;

	ASSERT_RESULT(KVulkanHelper::FilterModeToVkFilter(m_MagFilter, magFilter));
	ASSERT_RESULT(KVulkanHelper::FilterModeToVkFilter(m_MinFilter, minFilter));

	VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;

	ASSERT_RESULT(KVulkanHelper::AddressModeToVkSamplerAddressMode(m_AddressU, addressModeU));
	ASSERT_RESULT(KVulkanHelper::AddressModeToVkSamplerAddressMode(m_AddressV, addressModeV));
	ASSERT_RESULT(KVulkanHelper::AddressModeToVkSamplerAddressMode(m_AddressW, addressModeW));

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerInfo.magFilter = magFilter;
	samplerInfo.minFilter = minFilter;

	samplerInfo.addressModeU = addressModeU;
	samplerInfo.addressModeV = addressModeV;
	samplerInfo.addressModeW = addressModeW;

	samplerInfo.anisotropyEnable = m_AnisotropicEnable ? VK_TRUE : VK_FALSE;
	samplerInfo.maxAnisotropy = m_AnisotropicEnable ? static_cast<float>(m_AnisotropicCount) : 0.0f;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	/*
	float4 mipmapsample(float2 uv, float mipLodBias)
	{
		lod = getLodLevelFromScreenSize(); //smaller when the object is close, may be negative
		lod = clamp(lod + mipLodBias, minLod, maxLod);

		level = clamp(floor(lod), 0, texture.mipLevels - 1);  //clamped to the number of mip levels in the texture

		if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST)
		{
			color = sample(lod, level);
		}
		else
		{
			color = blend(sample(lod, level), sample(lod, level + 1));
		}
	}
	float4 sample(int lod, int level)
	{
		if (lod <= 0)
		{
			color = readTexture(level, uv, magFilter);
		}
		else
		{
			color = readTexture(level, uv, minFilter);
		}
	}
	*/
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = float(m_MinMipmap);
	samplerInfo.maxLod = float(m_MaxMipmap);

	VK_ASSERT_RESULT(vkCreateSampler(KVulkanGlobal::device, &samplerInfo, nullptr, &m_TextureSampler));
	m_SamplerInit = true;
	return true;
}

bool KVulkanSampler::UnInit()
{
	if(m_SamplerInit)
	{
		assert(KVulkanGlobal::deviceReady);
		vkDestroySampler(KVulkanGlobal::device, m_TextureSampler, nullptr);
		ZERO_MEMORY(m_TextureSampler);
		m_SamplerInit = false;
		return true;
	}
	return false;
}