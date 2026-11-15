#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Publish/KImage.h"

struct KCodecResult
{
	KImageDataPtr pData;

	PixelFormat eFormat;
	uint32_t uWidth;
	uint32_t uHeight;
	uint32_t uDepth;
	uint32_t uSlice;
	uint32_t uMipmap;

	bool bCompressed;
	bool bCubemap;
	bool b3DTexture;
	bool bTextureArray;

	KCodecResult()
	{
		pData = nullptr;
		eFormat = PF_UNKNOWN;
		uWidth = 0;
		uHeight = 0;
		uDepth = 1;
		uMipmap = 1;
		uSlice = 1;
		bCompressed = false;
		bCubemap = false;
		b3DTexture = false;
		bTextureArray = false;
	}
};

struct IKCodec;
typedef std::shared_ptr<IKCodec> IKCodecPtr;

struct IKCodec
{
	virtual bool Codec(const char* pszFile, bool forceAlpha, KCodecResult& result) = 0;
	virtual bool Codec(const char* pMemory, size_t size, bool forceAlpha, KCodecResult& result) = 0;
	virtual bool Save(const KCodecResult& source, const char* pszFile) = 0;
};

namespace KCodec
{
	extern bool ETC1HardwareCodec;
	extern bool ETC2HardwareCodec;
	extern bool ASTCHardwareCodec;
	extern bool BCHardwareCodec;

	extern bool QueryFormatHardwareDecode(PixelFormat format);

	extern bool CreateCodecManager();
	extern bool DestroyCodecManager();
	extern IKCodecPtr GetCodec(const char* pszFile);
}