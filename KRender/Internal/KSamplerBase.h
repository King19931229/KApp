#pragma once
#include "Interface/IKSampler.h"

class KSamplerBase : public IKSampler
{
protected:
	AddressMode m_AddressU;
	AddressMode m_AddressV;
	AddressMode m_AddressW;

	FilterMode m_MinFilter;
	FilterMode m_MagFilter;

	bool m_AnisotropicEnable;
	unsigned short m_AnisotropicCount;

	unsigned short m_MinMipmap;
	unsigned short m_MaxMipmap;
public:
	KSamplerBase();
	virtual ~KSamplerBase();

	virtual bool SetAddressMode(AddressMode addressU, AddressMode addressV, AddressMode addressW);
	virtual bool GetAddressMode(AddressMode& addressU, AddressMode& addressV, AddressMode& addressW);

	virtual bool SetFilterMode(FilterMode minFilter, FilterMode magFilter);
	virtual bool GetFilterMode(FilterMode& minFilter, FilterMode& magFilter);

	virtual bool SetAnisotropic(bool enable);
	virtual bool GetAnisotropic(bool& enable);

	virtual bool SetAnisotropicCount(unsigned short count);
	virtual bool GetAnisotropicCount(unsigned short& count);

	virtual bool GetMipmapLod(unsigned short& minMipmap, unsigned short& maxMipmap);

	virtual bool Init(unsigned short minMipmap, unsigned short maxMipmap) = 0;
	virtual bool UnInit() = 0;
};