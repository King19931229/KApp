#include "KVulkanSampler.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KBase/Interface/IKLog.h"

KVulkanSampler::KVulkanSampler()
	: KSamplerBase()
	, m_TextureSampler(VK_NULL_HANDLE)
{
}

KVulkanSampler::~KVulkanSampler()
{
	ASSERT_RESULT(m_TextureSampler == VK_NULL_HANDLE);
}

bool KVulkanSampler::ReleaseDevice()
{
	if (m_TextureSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(KVulkanGlobal::device, m_TextureSampler, nullptr);
		m_TextureSampler = VK_NULL_HANDLE;
	}
	return true;
}

bool KVulkanSampler::CreateDevice()
{
	assert(KVulkanGlobal::deviceReady);
	assert(m_TextureSampler == VK_NULL_HANDLE);

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

	if (KRenderGlobal::SupportAnisotropySample)
	{
		samplerInfo.anisotropyEnable = m_AnisotropicEnable ? VK_TRUE : VK_FALSE;
		samplerInfo.maxAnisotropy = m_AnisotropicEnable ? static_cast<float>(m_AnisotropicCount) : 0.0f;
	}
	else
	{
		if (m_AnisotropicEnable)
		{
			KG_LOGE(LM_RENDER, "Try to use anisotropic but not supported");
		}
		samplerInfo.anisotropyEnable = VK_FALSE;
	}

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
	return true;
}

bool KVulkanSampler::Init(unsigned short minMipmap, unsigned short maxMipmap)
{
	UnInit();
	m_MinMipmap = minMipmap;
	m_MaxMipmap = maxMipmap;
	ReleaseDevice();
	CreateDevice();
	return true;
}

bool KVulkanSampler::UnInit()
{
	ReleaseDevice();
	return true;
}