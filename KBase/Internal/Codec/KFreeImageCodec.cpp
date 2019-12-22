#include "KFreeImageCodec.h"
#include "FreeImage.h"
#include "Interface/IKDataStream.h"
#include "Interface/IKFileSystem.h"

// KFreeImageCodec
KFreeImageCodec::SupportExt KFreeImageCodec::ms_SupportExts;

KFreeImageCodec::KFreeImageCodec(int nType)
	: m_nType(nType)
{
}

KFreeImageCodec::~KFreeImageCodec()
{

}

bool KFreeImageCodec::Codec(const char* pszFile, bool forceAlpha, KCodecResult& result)
{
	bool bSuccess = false;
	
	IKDataStreamPtr pData = nullptr;
	FIMEMORY *fiMem = nullptr;
	FIBITMAP *fiBitmap = nullptr;

	result.uWidth = 0;
	result.uHeight = 0;
	result.eFormat = IF_INVALID;

	if(GFileSystemManager->Open(pszFile, IT_FILEHANDLE, pData))
	{
		std::vector<char> buffer;
		buffer.resize(pData->GetSize());

		if(pData->Read(buffer.data(), buffer.size()))
		{
			fiMem = FreeImage_OpenMemory((BYTE*)(buffer.data()), static_cast<DWORD>(pData->GetSize()));
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
				{
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
					else if (bpp < 8|| colourType == FIC_PALETTE || colourType == FIC_CMYK)
					{
						FIBITMAP *newBitmap = FreeImage_ConvertTo24Bits(fiBitmap);
						// free old bitmap and replace
						FreeImage_Unload(fiBitmap);
						fiBitmap = newBitmap;
						// get new formats
						bpp = FreeImage_GetBPP(fiBitmap);
						colourType = FreeImage_GetColorType(fiBitmap);
					}

					// Extra logic forcing alpha
					if(bpp < 32 && forceAlpha)
					{
						FIBITMAP *newBitmap = FreeImage_ConvertTo32Bits(fiBitmap);
						// free old bitmap and replace
						FreeImage_Unload(fiBitmap);
						fiBitmap = newBitmap;
						// get new formats
						bpp = FreeImage_GetBPP(fiBitmap);
						colourType = FreeImage_GetColorType(fiBitmap);
					}
				}
			default:
				{
					bpp = 0;
					assert(false && "unsupport to perform now");
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

				bSuccess = true;
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

	if(bSuccess)
	{
		KSubImageInfo subImageInfo;
		subImageInfo.uFaceIndex = 0;
		subImageInfo.uMipmapIndex = 0;
		subImageInfo.uOffset = 0;
		subImageInfo.uSize = result.pData->GetSize();
		subImageInfo.uWidth = result.uWidth;
		subImageInfo.uHeight = result.uHeight;
		result.pData->GetSubImageInfo().push_back(subImageInfo);
	}

	return bSuccess;
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