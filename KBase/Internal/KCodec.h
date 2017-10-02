#pragma once
#include "Interface/IKCodec.h"
#include <unordered_map>

class KFreeImageCodec : public IKCodec
{
protected:
	CodecResult m_Result;
public:
	KFreeImageCodec();
	virtual ~KFreeImageCodec();

	virtual bool Codec(const char* pszFile);
	virtual bool Clear();
	virtual CodecResult GetResult();

	static bool Init();
	static bool UnInit();
	static bool Support(const char* pExt);
};

class KCodecManager
{
protected:
public:
	KCodecManager();
	~KCodecManager();

	bool Init();
	bool UnInit();

	IKCodecPtr GetCodec(const char* pFilePath);
};