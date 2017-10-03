#pragma once
#include "Interface/IKConfig.h"
#include <memory>
#include <vector>

enum ImageFormat
{
	IF_R8G8B8A8,
	IF_R8G8B8,

	IF_COUNT,
	IF_INVALID = IF_COUNT
};

typedef std::shared_ptr<std::vector<unsigned char>> ImageData;
struct CodecResult
{
	bool bSuccess;
	ImageFormat eFormat;
	size_t uWidth;
	size_t uHeight;
	ImageData pData;
	CodecResult()
	{
		bSuccess = false;
		eFormat = IF_INVALID;
		uWidth = 0;
		uHeight = 0;
	}
};

struct IKCodec;
typedef std::shared_ptr<IKCodec> IKCodecPtr;

struct IKCodec
{
	virtual CodecResult Codec(const char* pszFile) = 0;
};

EXPORT_DLL bool InitCodecManager();
EXPORT_DLL bool UnInitCodecManager();
EXPORT_DLL IKCodecPtr GetCodec(const char* pszFile);