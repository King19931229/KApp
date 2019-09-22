#pragma once
#include "IKRenderConfig.h"

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

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
};