#pragma once
#include "Internal/KSamplerBase.h"
#include "KVulkanConfig.h"

class KVulkanSampler : public KSamplerBase
{
protected:
	VkSampler m_TextureSampler;
	bool m_SamplerInit;
public:
	KVulkanSampler();
	virtual ~KVulkanSampler();

	virtual bool Init();
	virtual bool UnInit();

	VkSampler GetVkSampler() { return m_TextureSampler; }
};