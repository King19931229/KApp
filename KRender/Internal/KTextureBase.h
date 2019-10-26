#pragma once
#include "Interface/IKTexture.h"
#include "KBase/Interface/IKCodec.h"

class KTextureBase : public IKTexture
{
protected:
	KCodecResult m_ImageData;
	ElementFormat m_Format;
	TextureType m_TextureType;
	size_t m_Width;
	size_t m_Height;
	size_t m_Depth;
	// mipmap层数
	unsigned short m_Mipmaps;
	// 是否需要硬生成mipmap
	bool m_bGenerateMipmap;
	bool m_bCreateAsRt;

	bool InitProperty(bool generateMipmap);
public:
	KTextureBase();
	virtual ~KTextureBase();

	virtual bool InitMemoryFromFile(const std::string& filePath, bool bGenerateMipmap);
	virtual bool InitMemoryFromData(const void* pRawData, size_t width, size_t height, ImageFormat format, bool bGenerateMipmap);
	virtual bool InitMemeoryAsRT(size_t width, size_t height, ElementFormat format);
	virtual bool InitDevice() = 0;
	virtual bool UnInit();

	virtual size_t GetWidth() { return m_Width; }
	virtual size_t GetHeight() { return m_Height; }
	virtual size_t GetDepth() { return m_Depth; }
	virtual unsigned short GetMipmaps() { return m_Mipmaps; }

	virtual TextureType GetTextureType() { return m_TextureType; }
	virtual ElementFormat GetTextureFormat() { return m_Format; }
};