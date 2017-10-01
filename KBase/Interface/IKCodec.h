#pragma once
#include "Interface/IKConfig.h"
#include <memory>
#include <vector>

enum ImageFormat
{
	IF_R8G8B8A8,
	IF_R8G8B8,

	IF_COUNT
};

typedef std::shared_ptr<std::vector<void*>> ImageData;
struct CodecResult
{
	ImageFormat eFormat;
	size_t uWidth;
	size_t uHeight;
	ImageData pData;
};

struct IKCodec;
typedef std::shared_ptr<IKCodec> IKCodecPtr;

struct IKCodec
{
	virtual bool Codec(const char* pszFile) = 0;
	virtual bool Clear() = 0;
	virtual CodecResult GetResult() = 0;
};

//EXPORT_DLL IKCodecPtr GetCodec();