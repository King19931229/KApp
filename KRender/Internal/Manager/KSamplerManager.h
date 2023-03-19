#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKSampler.h"
#include <unordered_map>

class KSamplerManager
{
protected:
	typedef std::unordered_map<size_t, KSamplerRef> SamplerMap;
	SamplerMap m_Samplers;
	KSamplerRef m_ErrorSampler;
	IKRenderDevice* m_Device;
	bool Release(IKSamplerPtr& sampler);
public:
	KSamplerManager();
	~KSamplerManager();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Acquire(const KSamplerDescription& desc, KSamplerRef& ref);
	bool GetErrorSampler(KSamplerRef& ref);
};