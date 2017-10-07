#include "Internal/KCodec.h"
#include "Interface/IKDataStream.h"
#include "FreeImage.h"

#include <string>
#include <algorithm>
#include <assert.h>

// KFreeImageCodec
KFreeImageCodec::SupportExt KFreeImageCodec::ms_SupportExts;

KFreeImageCodec::KFreeImageCodec(int nType)
	: m_nType(nType)
{
}

KFreeImageCodec::~KFreeImageCodec()
{

}

KCodecResult KFreeImageCodec::Codec(const char* pszFile)
{
	KCodecResult result;

	IKDataStreamPtr pData = GetDataStream(IT_MEMORY);
	FIMEMORY *fiMem = nullptr;
	FIBITMAP *fiBitmap = nullptr;

	result.bSuccess = false;
	result.uWidth = 0;
	result.uHeight = 0;
	result.eFormat = IF_INVALID;

	if(pData->Open(pszFile, IM_READ))
	{
		char* pRefData = nullptr;
		size_t uSize = pData->GetSize();
		if(pData->Reference(&pRefData, uSize))
		{
			fiMem = FreeImage_OpenMemory((BYTE*)(pRefData), static_cast<DWORD>(pData->GetSize()));
			fiBitmap = FreeImage_LoadFromMemory((FREE_IMAGE_FORMAT)m_nType, fiMem);

			unsigned bpp = FreeImage_GetBPP(fiBitmap);
			FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(fiBitmap);
			FREE_IMAGE_COLOR_TYPE colourType = FreeImage_GetColorType(fiBitmap);

			result.uWidth = FreeImage_GetWidth(fiBitmap);
			result.uHeight = FreeImage_GetHeight(fiBitmap);

			// Copy Form OGRE
			switch(imageType)
			{
			case FIT_BITMAP:
				// Standard image type
				// Perform any colour conversions for greyscale
				if (colourType == FIC_MINISWHITE || colourType == FIC_MINISBLACK)
				{
					FIBITMAP *newBitmap = FreeImage_ConvertToGreyscale(fiBitmap);
					FreeImage_Unload(fiBitmap);
					fiBitmap = newBitmap;
					bpp = FreeImage_GetBPP(fiBitmap);
					colourType = FreeImage_GetColorType(fiBitmap);
				}

				// Perform any colour conversions for RGB
				else if (bpp < 8 || colourType == FIC_PALETTE || colourType == FIC_CMYK)
				{
					FIBITMAP *newBitmap = FreeImage_ConvertTo24Bits(fiBitmap);
					// free old bitmap and replace
					FreeImage_Unload(fiBitmap);
					fiBitmap = newBitmap;
					// get new formats
					bpp = FreeImage_GetBPP(fiBitmap);
					colourType = FreeImage_GetColorType(fiBitmap);
				}
			}

			switch(bpp)
			{
			case 24:
				result.eFormat = IF_R8G8B8;
				break;
			case 32:
				result.eFormat = IF_R8G8B8A8;
				break;
			case 8:
			case 16:
			default:
				result.eFormat = IF_INVALID;
				break;
			}

			if(result.eFormat != IF_INVALID)
			{
				unsigned char *srcData = FreeImage_GetBits(fiBitmap);
				assert(srcData);
				unsigned int srcPitch = FreeImage_GetPitch(fiBitmap);
				assert(srcPitch > 0);
				KImageDataPtr pData = KImageDataPtr(new KImageData(srcPitch * result.uHeight));
				memcpy((void*)pData->GetData(), (const void*)srcData, pData->GetSize());
				result.pData = pData;

				result.bSuccess = true;
			}
		}
	}

	if(fiBitmap)
	{
		FreeImage_Unload(fiBitmap);
		fiBitmap = nullptr;
	}

	if(fiMem)
	{
		FreeImage_CloseMemory(fiMem);
		fiMem = nullptr;
	}

	return result;
}

bool KFreeImageCodec::Init()
{
	FreeImage_Initialise(false);
	std::string ext;
	const char* pExts = nullptr;
	const char* pDotPos = nullptr;
	ms_SupportExts.clear();
	for(int i = 0; i < FreeImage_GetFIFCount(); ++i)
	{
		if ((FREE_IMAGE_FORMAT)i == FIF_DDS)
			continue;
		pExts = FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i);
		assert(pExts);

		IKCodecPtr pCodec = IKCodecPtr(new KFreeImageCodec(i));
		while(pExts)
		{
			pDotPos = strchr(pExts, ',');
			if(pDotPos)
			{
				ext = std::string(pExts, pDotPos - pExts);
				pExts = pDotPos + 1;
			}
			else
			{
				ext = std::string(pExts);
				pExts = pDotPos;
			}
			if(KCodecManager::AddCodec(ext.c_str(), pCodec))
			{
				ms_SupportExts.insert(ext);
			}
		}
	}
	return true;
}

bool KFreeImageCodec::UnInit()
{
	FreeImage_DeInitialise();
	for(const std::string& ext : ms_SupportExts)
	{
		KCodecManager::RemoveCodec(ext.c_str());
	}
	ms_SupportExts.clear();
	return true;
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

// EXPORT
EXPORT_DLL bool InitCodecManager()
{
	bool bRet = KCodecManager::Init();
	return bRet;
}

EXPORT_DLL bool UnInitCodecManager()
{
	bool bRet = KCodecManager::UnInit();
	return bRet;
}

EXPORT_DLL IKCodecPtr GetCodec(const char* pszFile)
{
	IKCodecPtr pRet = KCodecManager::GetCodec(pszFile);
	return pRet;
}