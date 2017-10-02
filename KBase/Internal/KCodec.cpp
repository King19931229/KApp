#include "Internal/KCodec.h"

KCodecManager g_CodecManager;

// KFreeImageCodec
KFreeImageCodec::KFreeImageCodec()
{

}

KFreeImageCodec::~KFreeImageCodec()
{

}

bool KFreeImageCodec::Codec(const char* pszFile)
{
	return false;
}

bool KFreeImageCodec::Clear()
{
	return true;
}

CodecResult KFreeImageCodec::GetResult()
{
	return m_Result;
}

bool KFreeImageCodec::Init()
{
	return true;
}

bool KFreeImageCodec::UnInit()
{
	return true;
}

bool KFreeImageCodec::Support(const char* pExt)
{
	return false;
}

// KCodecManager
KCodecManager::KCodecManager()
{

}

KCodecManager::~KCodecManager()
{
	UnInit();
}

bool KCodecManager::Init()
{
	bool bRet = true;
	bRet &= KFreeImageCodec::Init();
	return bRet;
}

bool KCodecManager::UnInit()
{
	bool bRet = true;
	bRet &= KFreeImageCodec::UnInit();
	return bRet;
}

IKCodecPtr KCodecManager::GetCodec(const char* pFilePath)
{
	IKCodecPtr pRet = nullptr;
	if(pFilePath)
	{
		const char* pExt = strrchr(pFilePath, '.');
		if(pExt++)
		{
			if(KFreeImageCodec::Support(pExt))
			{
				pRet = IKCodecPtr(new KFreeImageCodec());
			}
		}
	}
	return pRet;
}

// EXPORT
bool InitCodecManager()
{
	bool bRet = g_CodecManager.Init();
	return bRet;
}

bool UnInitCodecManager()
{
	bool bRet = g_CodecManager.UnInit();
	return bRet;
}

IKCodecPtr GetCodec(const char* pszFile)
{
	IKCodecPtr pRet = g_CodecManager.GetCodec(pszFile);
	return pRet;
}