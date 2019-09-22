#include "KSamplerBase.h"

KSamplerBase::KSamplerBase()
	: m_AddressU(AM_REPEAT),
	m_AddressV(AM_REPEAT),
	m_AddressW(AM_REPEAT),

	m_MinFilter(FM_NEAREST),
	m_MagFilter(FM_NEAREST),

	m_AnisotropicEnable(false),
	m_AnisotropicCount(0)
{

}

KSamplerBase::~KSamplerBase()
{

}

bool KSamplerBase::SetAddressMode(AddressMode addressU, AddressMode addressV, AddressMode addressW)
{
	m_AddressU = addressU;
	m_AddressV = addressV;
	m_AddressW = addressW;
	return true;
}
bool KSamplerBase::GetAddressMode(AddressMode& addressU, AddressMode& addressV, AddressMode& addressW)
{
	addressU = m_AddressU;
	addressV = m_AddressV;
	addressW = m_AddressW;
	return true;
}

bool KSamplerBase::SetFilterMode(FilterMode minFilter, FilterMode magFilter)
{
	m_MinFilter = minFilter;
	m_MagFilter = magFilter;
	return true;
}

bool KSamplerBase::GetFilterMode(FilterMode& minFilter, FilterMode& magFilter)
{
	minFilter = m_MinFilter;
	magFilter = m_MagFilter;
	return true;
}

bool KSamplerBase::SetAnisotropic(bool enable)
{
	m_AnisotropicEnable = enable;
	return true;
}

bool KSamplerBase::GetAnisotropic(bool& enable)
{
	enable = m_AnisotropicEnable;
	return true;
}

bool KSamplerBase::SetAnisotropicCount(unsigned short count)
{
	m_AnisotropicCount = count;
	return true;
}

bool KSamplerBase::GetAnisotropicCount(unsigned short& count)
{
	count = m_AnisotropicCount;
	return true;
}