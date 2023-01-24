#include "KFreeImageCodec.h"
#include "FreeImage.h"
#include "Interface/IKDataStream.h"
#include "Interface/IKFileSystem.h"
#include "Publish/KStringUtil.h"
#include "Publish/KFileTool.h"

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
	
	IKDataStreamPtr stream = nullptr;
	FIMEMORY *fiMem = nullptr;
	FIBITMAP *fiBitmap = nullptr;

	result.uWidth = 0;
	result.uHeight = 0;
	result.eFormat = IF_INVALID;

	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
	if (!system || !system->Open(pszFile, IT_FILEHANDLE, stream))
	{
		system = KFileSystem::Manager->GetFileSystem(FSD_BACKUP);
		if (!system || !system->Open(pszFile, IT_FILEHANDLE, stream))
		{
			return false;
		}
	}

	{
		std::vector<char> buffer;
		buffer.resize(stream->GetSize());

		if(stream->Read(buffer.data(), buffer.size()))
		{
			fiMem = FreeImage_OpenMemory((BYTE*)(buffer.data()), static_cast<DWORD>(stream->GetSize()));
			fiBitmap = FreeImage_LoadFromMemory((FREE_IMAGE_FORMAT)m_nType, fiMem);

			unsigned bpp = FreeImage_GetBPP(fiBitmap);
			FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(fiBitmap);
			FREE_IMAGE_COLOR_TYPE colourType = FreeImage_GetColorType(fiBitmap);

			result.uWidth = FreeImage_GetWidth(fiBitmap);
			result.uHeight = FreeImage_GetHeight(fiBitmap);
			result.eFormat = IF_INVALID;

			// Copy Form OGRE
			switch(imageType)
			{
				// Standard image type
				case FIT_BITMAP:
				{
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

						imageType = FreeImage_GetImageType(fiBitmap);
						colourType = FreeImage_GetColorType(fiBitmap);
					}

					switch (bpp)
					{
						case 24:
							result.eFormat = IF_R8G8B8;
							break;
						case 32:
							result.eFormat = IF_R8G8B8A8;
							break;
						case 8:
							//result.eFormat = IF_R8;
							//break;
						case 16:
							//result.eFormat = IF_R8G8;
							//break;
						default:
							result.eFormat = IF_INVALID;
							break;
					}
					break;
				}
				// Special int image type
				case FIT_UINT16:
				case FIT_INT16:
				case FIT_UINT32:
				case FIT_INT32:
				{
					// Convert into float for simplicity
					FIBITMAP* newBitmap = FreeImage_ConvertToFloat(fiBitmap);
					FreeImage_Unload(fiBitmap);
					fiBitmap = newBitmap;
					bpp = FreeImage_GetBPP(fiBitmap);

					imageType = FreeImage_GetImageType(fiBitmap);
					colourType = FreeImage_GetColorType(fiBitmap);

					result.eFormat = IF_R32_FLOAT;
					break;
				}
				case FIT_FLOAT:
				{
					result.eFormat = IF_R32_FLOAT;
					break;
				}
				case FIT_RGBF:
				{
					result.eFormat = IF_R32G32B32_FLOAT;
					if (forceAlpha)
					{
						FIBITMAP* newBitmap = FreeImage_ConvertToRGBAF(fiBitmap);
						FreeImage_Unload(fiBitmap);
						fiBitmap = newBitmap;
						bpp = FreeImage_GetBPP(fiBitmap);

						imageType = FreeImage_GetImageType(fiBitmap);
						colourType = FreeImage_GetColorType(fiBitmap);

						result.eFormat = IF_R32G32B32A32_FLOAT;
					}
					break;
				}
				case FIT_RGBAF:
				{
					result.eFormat = IF_R32G32B32_FLOAT;
					break;
				}
				// keep the compiler happy
				default:
				{
					break;
				}
			}

			if(result.eFormat != IF_INVALID)
			{
				unsigned char *srcData = FreeImage_GetBits(fiBitmap);
				assert(srcData);
				unsigned int srcPitch = FreeImage_GetPitch(fiBitmap);
				assert(srcPitch > 0);
				KImageDataPtr pData = KImageDataPtr(KNEW KImageData(srcPitch * result.uHeight));
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

bool KFreeImageCodec::Save(const KCodecResult& source, const char* pszFile)
{
	bool bSuccess = false;

	std::string name, ext;

	ASSERT_RESULT(pszFile && KFileTool::SplitExt(pszFile, name, ext));
	ASSERT_RESULT(KStringUtil::Lower(ext, ext));

	if (ext.length() && ext[0] == '.')
		ext = ext.substr(1);

	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFormat(ext.c_str());

	if (source.eFormat == IF_R8G8B8A8 && fif != FIF_UNKNOWN && ms_SupportExts.find(ext) != ms_SupportExts.end())
	{
		size_t byteSize = 0;
		ASSERT_RESULT(KImageHelper::GetElementByteSize(source.eFormat, byteSize));
		BYTE* bits = source.pData->GetData();

		int bpp = (int)byteSize * 8;
		int width = (int)source.uWidth;
		int height = (int)source.uHeight;
		int pitch = width * (int)byteSize;

		FIBITMAP* dib = FreeImage_ConvertFromRawBits(bits, width, height, pitch, bpp, 0, 0, 0);
		FIMEMORY* foMem = FreeImage_OpenMemory();

		if (FreeImage_SaveToMemory(fif, dib, foMem))
		{
			BYTE* writeData = NULL;
			DWORD writeSize = 0;
			FreeImage_AcquireMemory(foMem, &writeData, &writeSize);
			IKDataStreamPtr stream = GetDataStream(IT_FILEHANDLE);
			stream->Open(pszFile, IM_WRITE);
			stream->Write(writeData, writeSize);
			stream->Close();
			bSuccess = true;
		}

		FreeImage_Unload(dib);
		FreeImage_CloseMemory(foMem);
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

		IKCodecPtr pCodec = IKCodecPtr(KNEW KFreeImageCodec(i));
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