#pragma once
#include "Internal/KCodec.h"
#include "Interface/IKDataStream.h"

class KCodecBase : public IKCodec
{
protected:
	virtual bool CodecImpl(IKDataStreamPtr stream, bool forceAlpha, KCodecResult& result) = 0;
public:
	virtual bool Codec(const char* pszFile, bool forceAlpha, KCodecResult& result);
	virtual bool Codec(const char* pMemory, size_t size, bool forceAlpha, KCodecResult& result);
};