#include "KVulkanSampler.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

KVulkanSampler::KVulkanSampler()
	: KSamplerBase(),
	m_TextureSampler(VK_NULL_HANDLE),
	m_DeviceLoadTask(nullptr),
	m_ResourceState(RS_UNLOADED)
{
}

KVulkanSampler::~KVulkanSampler()
{
	ASSERT_RESULT(m_TextureSampler == VK_NULL_HANDLE);
	ASSERT_RESULT(m_DeviceLoadTask == nullptr);
	ASSERT_RESULT(m_ResourceState == RS_UNLOADED);
}

ResourceState KVulkanSampler::GetResourceState()
{
	return m_ResourceState;
}

void KVulkanSampler::WaitForMemory()
{
	return;
}

void KVulkanSampler::WaitForDevice()
{
	WaitDeviceTask();
}

bool KVulkanSampler::CancelDeviceTask()
{
	std::unique_lock<decltype(m_DeviceLoadTaskLock)> guard(m_DeviceLoadTaskLock);
	if (m_DeviceLoadTask)
	{
		m_DeviceLoadTask->Cancel();
		m_DeviceLoadTask = nullptr;
	}
	return true;
}

bool KVulkanSampler::WaitDeviceTask()
{
	std::unique_lock<decltype(m_DeviceLoadTaskLock)> guard(m_DeviceLoadTaskLock);
	if (m_DeviceLoadTask)
	{
		m_DeviceLoadTask->Wait();
		m_DeviceLoadTask = nullptr;
	}
	return true;
}

bool KVulkanSampler::ReleaseDevice()
{
	CancelDeviceTask();
	if (m_TextureSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(KVulkanGlobal::device, m_TextureSampler, nullptr);
		m_TextureSampler = VK_NULL_HANDLE;
	}
	m_ResourceState = RS_UNLOADED;
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
	return true;
}

bool KVulkanSampler::Init(unsigned short minMipmap, unsigned short maxMipmap)
{
	m_MinMipmap = minMipmap;
	m_MaxMipmap = maxMipmap;
	ReleaseDevice();
	CreateDevice();
	m_ResourceState = RS_DEVICE_LOADED;
	return true;
}

bool KVulkanSampler::Init(IKTexturePtr texture, bool async)
{
	if (texture)
	{
		ReleaseDevice();

		auto loadImpl = [=]()->bool
		{
			m_ResourceState = RS_DEVICE_LOADING;
			texture->WaitForDevice();
			m_MinMipmap = 0;
			m_MaxMipmap = texture->GetMipmaps();
			CreateDevice();
			m_ResourceState = RS_DEVICE_LOADED;
			return true;
		};

		if (async)
		{
			m_ResourceState = RS_PENDING;
			std::unique_lock<decltype(m_DeviceLoadTaskLock)> guard(m_DeviceLoadTaskLock);
			m_DeviceLoadTask = KRenderGlobal::TaskExecutor.Submit(KTaskUnitPtr(new KSampleAsyncTaskUnit(loadImpl)));
			return true;
		}
		else
		{
			return loadImpl();
		}
	}
	return false;
}

bool KVulkanSampler::UnInit()
{
	ReleaseDevice();
	return true;
}