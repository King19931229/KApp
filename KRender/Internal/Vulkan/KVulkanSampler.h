#pragma once
#include "Internal/KSamplerBase.h"
#include "Internal/KRenderGlobal.h"
#include "KVulkanConfig.h"

class KVulkanSampler : public KSamplerBase
{
protected:
	VkSampler m_TextureSampler;

	std::mutex m_LoadTaskLock;
	KTaskUnitProcessorPtr m_LoadTask;;

	ResourceState m_ResourceState;

	bool CancelDeviceTask();
	bool WaitDeviceTask();
	bool ReleaseDevice();
	bool CreateDevice();
public:
	KVulkanSampler();
	virtual ~KVulkanSampler();

	virtual ResourceState GetResourceState();
	virtual void WaitForMemory();
	virtual void WaitForDevice();

	virtual bool Init(unsigned short minMipmap, unsigned short maxMipmap);
	// TODO 改为轮询重建
	virtual bool Init(IKTexturePtr texture, bool async);

	virtual bool UnInit();

	VkSampler GetVkSampler() { return m_TextureSampler; }
};