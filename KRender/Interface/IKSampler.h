#pragma once
#include "IKRenderConfig.h"
#include "IKResource.h"
#include "KBase/Publish/KReferenceHolder.h"
#include <glm/glm.hpp>

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

	glm::vec4 borderColor;

	KSamplerDescription()
	{
		// 与KSamplerBase匹配
		ZERO_MEMORY(*this);
		addressU = addressV = addressW = AM_REPEAT;
		minFilter = magFilter = FM_NEAREST;
		minMipmap = 0;
		maxMipmap = 0;
		anisotropicCount = 0;
		anisotropic = false;
		borderColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
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

struct IKSampler
{
	virtual ~IKSampler() {}

	virtual bool SetAddressMode(AddressMode addressU, AddressMode addressV, AddressMode addressW) = 0;
	virtual bool GetAddressMode(AddressMode& addressU, AddressMode& addressV, AddressMode& addressW) = 0;

	virtual bool SetFilterMode(FilterMode minFilter, FilterMode magFilter) = 0;
	virtual bool GetFilterMode(FilterMode& minFilter, FilterMode& magFilter) = 0;

	virtual bool SetAnisotropic(bool enable) = 0;
	virtual bool GetAnisotropic(bool& enable) = 0;

	virtual bool SetAnisotropicCount(unsigned short count) = 0;
	virtual bool GetAnisotropicCount(unsigned short& count) = 0;

	virtual bool SetBorderColor(const glm::vec4& color) = 0;
	virtual bool GetBorderColor(glm::vec4& color) = 0;

	virtual bool GetMipmapLod(unsigned short& minMipmap, unsigned short& maxMipmap) = 0;

	virtual bool Init(unsigned short minMipmap, unsigned short maxMipmap) = 0;

	virtual bool UnInit() = 0;
};

typedef KReferenceHolder<IKSamplerPtr> KSamplerRef;