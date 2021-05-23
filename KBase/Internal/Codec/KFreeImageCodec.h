#pragma once
#include "Internal/KCodec.h"
#include <unordered_set>
#include <string>

class KFreeImageCodec : public IKCodec
{
public:
	typedef std::unordered_set<std::string> SupportExt;
protected:
	int m_nType;
	static SupportExt ms_SupportExts;
public:
	KFreeImageCodec(int nType);
	virtual ~KFreeImageCodec();

	virtual bool Codec(const char* pszFile, bool forceAlpha, KCodecResult& result);
	virtual bool Save(const KCodecResult& source, const char* pszFile);

	static bool Init();
	static bool UnInit();
};