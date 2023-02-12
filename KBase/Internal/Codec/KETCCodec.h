#pragma once
#include "KCodecBase.h"

class KETCCodec : public KCodecBase
{
protected:
	virtual bool CodecImpl(IKDataStreamPtr stream, bool forceAlpha, KCodecResult& result) override;
public:
	KETCCodec();
	virtual ~KETCCodec();

	bool DecodePKM(const IKDataStreamPtr& stream, KCodecResult& result);
	bool DecodeKTX(const IKDataStreamPtr& stream, KCodecResult& result);

	virtual bool Save(const KCodecResult& source, const char* pszFile);

	static bool Init();
	static bool UnInit();
};