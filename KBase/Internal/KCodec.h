#pragma once
#include "Interface/IKCodec.h"

#include <set>
#include <map>
#include <mutex>

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

	bool Codec(const char* pszFile, bool forceAlpha, KCodecResult& result);

	static bool Init();
	static bool UnInit();
};

class KCodecManager
{
public:
	typedef std::map<std::string, IKCodecPtr> SupportCodec;
public:
	KCodecManager();
	~KCodecManager();

	static std::mutex ms_Lock;
	static SupportCodec ms_Codecs;

	static bool Init();
	static bool UnInit();

	static bool AddCodec(const char* pExt, IKCodecPtr pCodec);
	static bool RemoveCodec(const char* pExt);

	static IKCodecPtr GetCodec(const char* pFilePath);
};