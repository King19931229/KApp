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

	bool QueryFormatHardwareDecode(ImageFormat format)
	{
		if (BCFormat(format))
		{
			return KCodec::BCHardwareCodec;
		}
		else if (ETC1Format(format))
		{
			return KCodec::ETC1HardwareCodec;
		}
		else if(ETC2Format(format))
		{
			return KCodec::ETC2HardwareCodec;
		}
		else if (ASTCFormat(format))
		{
			return KCodec::ASTCHardwareCodec;
		}
		return true;
	}

	bool BCFormat(ImageFormat format)
	{
		switch (format)
		{
			case IF_DXT1:
			case IF_DXT2:
			case IF_DXT3:
			case IF_DXT4:
			case IF_DXT5:
			case IF_BC4_UNORM:
			case IF_BC4_SNORM:
			case IF_BC5_UNORM:
			case IF_BC5_SNORM:
			case IF_BC6H_UF16:
			case IF_BC6H_SF16:
			case IF_BC7_UNORM:
			case IF_BC7_UNORM_SRGB:
				return true;
			default:
				return false;
		}
	}

	bool ETC1Format(ImageFormat format)
	{
		switch (format)
		{
			case IF_ETC1_RGB8:
				return true;
			default:
				return false;
		}
	}

	bool ETC2Format(ImageFormat format)
	{
		switch (format)
		{
			case IF_ETC2_RGB8:
			case IF_ETC2_RGB8A8:
			case IF_ETC2_RGB8A1:
				return true;
			default:
				return false;
		}
	}

	bool ASTCFormat(ImageFormat format)
	{
		switch (format)
		{
			case IF_ASTC_4x4_UNORM:
			case IF_ASTC_4x4_SRGB:
			case IF_ASTC_5x4_UNORM:
			case IF_ASTC_5x4_SRGB:
			case IF_ASTC_5x5_UNORM:
			case IF_ASTC_5x5_SRGB:
			case IF_ASTC_6x5_UNORM:
			case IF_ASTC_6x5_SRGB:
			case IF_ASTC_6x6_UNORM:
			case IF_ASTC_6x6_SRGB:
			case IF_ASTC_8x5_UNORM:
			case IF_ASTC_8x5_SRGB:
			case IF_ASTC_8x6_UNORM:
			case IF_ASTC_8x6_SRGB:
			case IF_ASTC_8x8_UNORM:
			case IF_ASTC_8x8_SRGB:
			case IF_ASTC_10x5_UNORM:
			case IF_ASTC_10x5_SRGB:
			case IF_ASTC_10x6_UNORM:
			case IF_ASTC_10x6_SRGB:
			case IF_ASTC_10x8_UNORM:
			case IF_ASTC_10x8_SRGB:
			case IF_ASTC_10x10_UNORM:
			case IF_ASTC_10x10_SRGB:
			case IF_ASTC_12x10_UNORM:
			case IF_ASTC_12x10_SRGB:
			case IF_ASTC_12x12_UNORM:
			case IF_ASTC_12x12_SRGB:
				return true;
			default:
				return false;
		}
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
	bool bSuccess = true;
	bSuccess &= KFreeImageCodec::Init();
	bSuccess &= KETCCodec::Init();
	bSuccess &= KDDSCodec::Init();
	return bSuccess;
}

bool KCodecManager::UnInit()
{
	bool bSuccess = true;
	bSuccess &= KFreeImageCodec::UnInit();
	bSuccess &= KETCCodec::UnInit();
	bSuccess &= KDDSCodec::UnInit();
	return bSuccess;
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