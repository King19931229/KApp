#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Publish/KImage.h"

struct KCodecResult
{
	KImageDataPtr pData;

	ImageFormat eFormat;
	size_t uWidth;
	size_t uHeight;
	size_t uDepth;
	size_t uMipmap;

	bool bCompressed;
	bool bCubemap;
	bool b3DTexture;

	KCodecResult()
	{
		pData = nullptr;
		eFormat = IF_INVALID;
		uWidth = 0;
		uHeight = 0;
		uDepth = 1;
		uMipmap = 1;
		bCompressed = false;
		bCubemap = false;
		b3DTexture = false;
	}
};

struct IKCodec;
typedef std::shared_ptr<IKCodec> IKCodecPtr;

struct IKCodec
{
	virtual bool Codec(const char* pszFile, bool forceAlpha, KCodecResult& result) = 0;
};

namespace KCodec
{
	extern bool ETC1HardwareCodec;
	extern bool ETC2HardwareCodec;
	extern bool ASTCHardwareCodec;
	extern bool BCHardwareCodec;

	extern bool CreateCodecManager();
	extern bool DestroyCodecManager();
	extern IKCodecPtr GetCodec(const char* pszFile);
}