#pragma once
#include "Internal/KCodec.h"
#include "Interface/IKDataStream.h"

class KETCCodec : public IKCodec
{
public:
	KETCCodec();
	virtual ~KETCCodec();

	bool DecodePKM(const IKDataStreamPtr& stream, KCodecResult& result);
	bool DecodeKTX(const IKDataStreamPtr& stream, KCodecResult& result);

	virtual bool Codec(const char* pszFile, bool forceAlpha, KCodecResult& result);
	virtual bool Save(const KCodecResult& source, const char* pszFile);

	static bool Init();
	static bool UnInit();
};