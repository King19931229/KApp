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
	bool Release(IKSamplerPtr& sampler);
public:
	KSamplerManager();
	~KSamplerManager();

	bool Init();
	bool UnInit();

	bool Acquire(const KSamplerDescription& desc, KSamplerRef& ref);
	bool GetErrorSampler(KSamplerRef& ref);
};