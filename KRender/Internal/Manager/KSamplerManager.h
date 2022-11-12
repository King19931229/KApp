#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKSampler.h"
#include <unordered_map>

struct KSamplerDescription
{
	AddressMode addressU;
	AddressMode addressV;
	AddressMode addressW;

	FilterMode minFilter;
	FilterMode magFilter;

	unsigned short minMipmap;
	unsigned short maxMipmap;

	unsigned short anisotropicCount;
	bool anisotropic;

	KSamplerDescription()
	{
		// 与KSamplerBase匹配
		addressU = addressV = addressW = AM_REPEAT;
		minFilter = magFilter = FM_NEAREST;
		minMipmap = 0;
		maxMipmap = 0;
		anisotropicCount = 0;
		anisotropic = false;
	}

	bool operator==(const KSamplerDescription& rhs) const
	{
		return memcmp(this, &rhs, sizeof(*this)) == 0;
	}
};

namespace std
{
	template<>
	struct hash<KSamplerDescription>
	{
		size_t operator()(const KSamplerDescription& desc) const;
	};
}

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