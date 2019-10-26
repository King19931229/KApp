#pragma once
#include "Interface/IKCodec.h"

#include <map>
#include <mutex>

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