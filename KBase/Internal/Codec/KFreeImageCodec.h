#pragma once
#include "KCodecBase.h"
#include <unordered_set>
#include <string>

class KFreeImageCodec : public KCodecBase
{
public:
	typedef std::unordered_set<std::string> SupportExt;
protected:
	int m_nType;
	static SupportExt ms_SupportExts;
	virtual bool CodecImpl(IKDataStreamPtr stream, bool forceAlpha, KCodecResult& result) override;
public:
	KFreeImageCodec(int nType);
	virtual ~KFreeImageCodec();

	virtual bool Save(const KCodecResult& source, const char* pszFile);

	static bool Init();
	static bool UnInit();
};