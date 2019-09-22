#include "KTextureBase.h"
#include "KBase/Interface/IKDataStream.h"

static bool ImageFormatToElementFormat(ImageFormat imageForamt, ElementFormat& elementFormat)
{
	switch (imageForamt)
	{
	case IF_R8G8B8A8:
		elementFormat = EF_R8GB8BA8_UNORM;
		return true;
	case IF_R8G8B8:
		elementFormat = EF_R8GB8B8_UNORM;
		return true;
	case IF_COUNT:
	default:
		elementFormat = EF_UNKNOWN;
		return false;
	}
}

KTextureBase::KTextureBase()
	: m_Width(0),
	m_Height(0),
	m_Depth(0),
	m_Format(EF_UNKNOWN),
	m_TextureType(TT_UNKNOWN)
{

}

KTextureBase::~KTextureBase()
{

}

bool KTextureBase::InitMemory(const std::string& filePath)
{
	bool bResult = false;
	IKCodecPtr pCodec = GetCodec(filePath.c_str());
	if(pCodec && pCodec->Codec(filePath.c_str(), true, m_ImageData))
	{
		m_Width = m_ImageData.uWidth;
		m_Height = m_ImageData.uHeight;
		m_Depth = 1;
		m_TextureType = TT_TEXTURE_2D;
		if(ImageFormatToElementFormat(m_ImageData.eFormat, m_Format))
		{
			bResult = true;
		}
	}

	if(!bResult)
	{
		m_ImageData.pData = nullptr;
	}

	return bResult;
}

bool KTextureBase::UnInit()
{
	m_ImageData.pData = nullptr;
	return true;
}