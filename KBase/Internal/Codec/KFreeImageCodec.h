#pragma once
#include "Internal/KCodec.h"
#include <set>
#include <string>

class KFreeImageCodec : public IKCodec
{
public:
	typedef std::set<std::string> SupportExt;
protected:
	int m_nType;
	static SupportExt ms_SupportExts;
public:
	KFreeImageCodec(int nType);
	virtual ~KFreeImageCodec();

	virtual bool Codec(const char* pszFile, bool forceAlpha, KCodecResult& result);

	static bool Init();
	static bool UnInit();
};