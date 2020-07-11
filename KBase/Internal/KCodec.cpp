#include "Internal/KCodec.h"

#include "Codec/KFreeImageCodec.h"
#include "Codec/KETCCodec.h"
#include "Codec/KDDSCodec.h"

#include <string>
#include <algorithm>
#include <assert.h>

namespace KCodec
{
	bool ETC1HardwareCodec = false;
	bool ETC2HardwareCodec = false;
	bool ASTCHardwareCodec = false;
	bool BCHardwareCodec = false;

	bool CreateCodecManager()
	{
		bool bRet = KCodecManager::Init();
		return bRet;
	}

	bool DestroyCodecManager()
	{
		bool bRet = KCodecManager::UnInit();
		return bRet;
	}

	IKCodecPtr GetCodec(const char* pszFile)
	{
		IKCodecPtr pRet = KCodecManager::GetCodec(pszFile);
		return pRet;
	}
}

// KCodecManager
KCodecManager::SupportCodec KCodecManager::ms_Codecs;
std::mutex KCodecManager::ms_Lock;

KCodecManager::KCodecManager()
{

}

KCodecManager::~KCodecManager()
{

}

bool KCodecManager::Init()
{
	if(KFreeImageCodec::Init() && KETCCodec::Init() && KDDSCodec::Init())
	{
		return true;
	}
	return false;
}

bool KCodecManager::UnInit()
{
	if(KFreeImageCodec::UnInit() && KETCCodec::UnInit() && KDDSCodec::UnInit())
	{
		return true;
	}
	return false;
}

bool KCodecManager::AddCodec(const char* pExt, IKCodecPtr pCodec)
{
	std::lock_guard<decltype(ms_Lock)> guard(ms_Lock);
	SupportCodec::iterator it = ms_Codecs.find(pExt);
	if(it == ms_Codecs.end())
	{
		ms_Codecs[pExt] = pCodec;
		return true;
	}
	return false;
}

bool KCodecManager::RemoveCodec(const char* pExt)
{
	std::lock_guard<decltype(ms_Lock)> guard(ms_Lock);
	SupportCodec::iterator it = ms_Codecs.find(pExt);
	if(it != ms_Codecs.end())
	{
		ms_Codecs.erase(it);
		return true;
	}
	return false;
}

IKCodecPtr KCodecManager::GetCodec(const char* pFilePath)
{
	IKCodecPtr pRet = nullptr;
	if(pFilePath)
	{
		const char* pExt = strrchr(pFilePath, '.');
		if(pExt++)
		{
			std::string ext = pExt;
			std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

			std::lock_guard<decltype(ms_Lock)> guard(ms_Lock);

			SupportCodec::iterator it = ms_Codecs.find(ext);
			if(it != ms_Codecs.end())
				pRet = it->second;
		}
	}
	return pRet;
}