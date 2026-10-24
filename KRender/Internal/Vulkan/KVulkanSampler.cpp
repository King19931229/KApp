#include "KVulkanSampler.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"

static VkBorderColor ToVkBorderColor(const glm::vec4& color)
{
	if (color.a <= 0.0f)
	{
		return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	}
	if (color.r == 0.0f && color.g == 0.0f && color.b == 0.0f)
	{
		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	}
	KG_LOGW(LM_RENDER, "Custom border color is not supported, fallback to opaque black");
	return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
}

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

	VkSamplerCustomBorderColorCreateInfoEXT customBorderColorInfo = {};
	if (KRenderGlobal::SupportCustomBorderColor)
	{
		customBorderColorInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
		customBorderColorInfo.customBorderColor.float32[0] = m_BorderColor.r;
		customBorderColorInfo.customBorderColor.float32[1] = m_BorderColor.g;
		customBorderColorInfo.customBorderColor.float32[2] = m_BorderColor.b;
		customBorderColorInfo.customBorderColor.float32[3] = m_BorderColor.a;
		customBorderColorInfo.format = VK_FORMAT_UNDEFINED;
		samplerInfo.pNext = &customBorderColorInfo;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	}
	else
	{
		samplerInfo.borderColor = ToVkBorderColor(m_BorderColor);
	}

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