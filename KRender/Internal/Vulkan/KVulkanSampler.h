#pragma once
#include "Internal/KSamplerBase.h"
#include "Internal/KRenderGlobal.h"
#include "KVulkanConfig.h"

class KVulkanSampler : public KSamplerBase
{
protected:
	VkSampler m_TextureSampler;

	bool ReleaseDevice();
	bool CreateDevice();
public:
	KVulkanSampler();
	virtual ~KVulkanSampler();

	virtual bool Init(unsigned short minMipmap, unsigned short maxMipmap);
	virtual bool UnInit();

	VkSampler GetVkSampler() { return m_TextureSampler; }
};