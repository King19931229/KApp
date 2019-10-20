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
		assert(false && "unsupport format");
		elementFormat = EF_UNKNOWN;
		return false;
	}
}

static bool ImageFormatToSize(ImageFormat format, size_t& size)
{
	switch (format)
	{
	case IF_R8G8B8A8:
		size = 4;
		return true;
	case IF_R8G8B8:
		size = 3;
		return true;
	case IF_COUNT:
		break;
	default:
		break;
	}
	assert(false && "unsupport format");
	size = 0;
	return false;
}

KTextureBase::KTextureBase()
	: m_Width(0),
	m_Height(0),
	m_Depth(0),
	m_Mipmaps(0),
	m_Format(EF_UNKNOWN),
	m_TextureType(TT_UNKNOWN),
	m_bCreateAsRt(false)
{

}

KTextureBase::~KTextureBase()
{

}

bool KTextureBase::InitProperty(bool bGenerateMipmap)
{
	m_Width = m_ImageData.uWidth;
	m_Height = m_ImageData.uHeight;
	m_Depth = 1;
	m_Mipmaps = bGenerateMipmap ? (unsigned short)std::floor(std::log(std::min(m_Width, m_Height)) / std::log(2)) + 1 : 1;
	m_TextureType = TT_TEXTURE_2D;
	if(ImageFormatToElementFormat(m_ImageData.eFormat, m_Format))
	{
		return true;
	}
	return false;
}

bool KTextureBase::InitMemoryFromFile(const std::string& filePath, bool bGenerateMipmap)
{
	IKCodecPtr pCodec = GetCodec(filePath.c_str());
	if(pCodec && pCodec->Codec(filePath.c_str(), true, m_ImageData))
	{
		if(InitProperty(bGenerateMipmap))
		{
			m_bCreateAsRt = false;
			return true;
		}
	}
	m_ImageData.pData = nullptr;
	return false;
}

bool KTextureBase::InitMemoryFromData(const void* pRawData, size_t width, size_t height, ImageFormat format, bool bGenerateMipmap)
{
	if(pRawData)
	{
		size_t formatSize = 0;
		if(ImageFormatToSize(format, formatSize))
		{
			KImageDataPtr pImageData = KImageDataPtr(new KImageData(width * height * formatSize));			 
			memcpy(pImageData->GetData(), pRawData, pImageData->GetSize());
			m_ImageData.eFormat = format;
			m_ImageData.uWidth = width;
			m_ImageData.uHeight = height;
			m_ImageData.pData = pImageData;
			if(InitProperty(bGenerateMipmap))
			{
				m_bCreateAsRt = false;
				return true;
			}
		}
	}
	m_ImageData.pData = nullptr;
	return false;
}

bool KTextureBase::InitMemeoryAsRT(size_t width, size_t height, ElementFormat format)
{
	m_Width = width;
	m_Height = height;
	m_Depth = 1;
	m_Mipmaps = 1;
	m_Format = format;
	m_TextureType = TT_TEXTURE_2D;
	m_bCreateAsRt = true;
	m_ImageData.pData = nullptr;
	return true;
}

bool KTextureBase::UnInit()
{
	m_ImageData.pData = nullptr;
	return true;
}