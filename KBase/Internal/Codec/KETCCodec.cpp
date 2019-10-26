#include "KETCCodec.h"

#define KTX_EXT "ktx"
#define PKM_EXT "pkm"

// Copy From OGRE
#define FOURCC(c0, c1, c2, c3) (c0 | (c1 << 8) | (c2 << 16) | (c3 << 24))
#define KTX_ENDIAN_REF      (0x04030201)
#define KTX_ENDIAN_REF_REV  (0x01020304)

// In a PKM-file, the codecs are stored using the following identifiers
//
// identifier                         value               codec
// --------------------------------------------------------------------
// ETC1_RGB_NO_MIPMAPS                  0                 GL_ETC1_RGB8_OES
// ETC2PACKAGE_RGB_NO_MIPMAPS           1                 GL_COMPRESSED_RGB8_ETC2
// ETC2PACKAGE_RGBA_NO_MIPMAPS_OLD      2, not used       -
// ETC2PACKAGE_RGBA_NO_MIPMAPS          3                 GL_COMPRESSED_RGBA8_ETC2_EAC
// ETC2PACKAGE_RGBA1_NO_MIPMAPS         4                 GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
// ETC2PACKAGE_R_NO_MIPMAPS             5                 GL_COMPRESSED_R11_EAC
// ETC2PACKAGE_RG_NO_MIPMAPS            6                 GL_COMPRESSED_RG11_EAC
// ETC2PACKAGE_R_SIGNED_NO_MIPMAPS      7                 GL_COMPRESSED_SIGNED_R11_EAC
// ETC2PACKAGE_RG_SIGNED_NO_MIPMAPS     8                 GL_COMPRESSED_SIGNED_RG11_EAC

const uint32_t PKM_MAGIC = FOURCC('P', 'K', 'M', ' ');
const uint32_t KTX_MAGIC = FOURCC(0xAB, 0x4B, 0x54, 0x58);

typedef struct
{
	uint8_t  name[4];
	uint8_t  version[2];
	uint8_t  iTextureTypeMSB;
	uint8_t  iTextureTypeLSB;
	uint8_t  iPaddedWidthMSB;
	uint8_t  iPaddedWidthLSB;
	uint8_t  iPaddedHeightMSB;
	uint8_t  iPaddedHeightLSB;
	uint8_t  iWidthMSB;
	uint8_t  iWidthLSB;
	uint8_t  iHeightMSB;
	uint8_t  iHeightLSB;
} PKMHeader;

typedef struct
{
	uint8_t	    identifier[12];
	uint32_t    endianness;
	uint32_t    glType;
	uint32_t    glTypeSize;
	uint32_t    glFormat;
	uint32_t    glInternalFormat;
	uint32_t    glBaseInternalFormat;
	uint32_t    pixelWidth;
	uint32_t    pixelHeight;
	uint32_t    pixelDepth;
	uint32_t    numberOfArrayElements;
	uint32_t    numberOfFaces;
	uint32_t    numberOfMipmapLevels;
	uint32_t    bytesOfKeyValueData;
} KTXHeader;

KETCCodec::KETCCodec()
{

}

KETCCodec::~KETCCodec()
{

}

bool KETCCodec::DecodePKM(const IKDataStreamPtr& stream, KCodecResult& result)
{
	PKMHeader header;

	// Read the ETC header
	stream->Read((char*)&header, sizeof(PKMHeader));

	if (PKM_MAGIC != FOURCC(header.name[0], header.name[1], header.name[2], header.name[3]) ) // "PKM 10"
	{
		return false;
	}

	uint16_t width = (header.iWidthMSB << 8) | header.iWidthLSB;
	uint16_t height = (header.iHeightMSB << 8) | header.iHeightLSB;
	uint16_t paddedWidth = (header.iPaddedWidthMSB << 8) | header.iPaddedWidthLSB;
	uint16_t paddedHeight = (header.iPaddedHeightMSB << 8) | header.iPaddedHeightLSB;
	uint16_t type = (header.iTextureTypeMSB << 8) | header.iTextureTypeLSB;

	result.uDepth = 1;
	result.uWidth = width;
	result.uHeight = height;

	// File version 2.0 supports ETC2 in addition to ETC1
	if(header.version[0] == '2' && header.version[1] == '0')
	{
		switch (type)
		{
		case 0:
			result.eFormat = IF_ETC1_RGB8;
			break;

			// GL_COMPRESSED_RGB8_ETC2
		case 1:
			result.eFormat = IF_ETC2_RGB8;
			break;

			// GL_COMPRESSED_RGBA8_ETC2_EAC
		case 3:
			result.eFormat = IF_ETC2_RGB8A8;
			break;

			// GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
		case 4:
			result.eFormat = IF_ETC2_RGB8A1;
			break;

			// Default case is ETC1
		default:
			result.eFormat = IF_ETC1_RGB8;
			break;
		}
	}
	else
	{
		result.eFormat = IF_ETC1_RGB8;
	}

	// ETC has no support for mipmaps - malideveloper.com has a example
	// where the load mipmap levels from different external files
	result.uMipmap = 0;

	// ETC is a compressed format
	result.bCompressed = true;

	// Calculate total size from number of mipmaps, faces and size
	result.pData = KImageDataPtr(new KImageData((paddedWidth * paddedHeight) >> 1));

	// Now deal with the data
	stream->Read((char*)result.pData->GetData(), result.pData->GetSize());

	KSubImageInfo subImageInfo;
	subImageInfo.uFaceIndex = 0;
	subImageInfo.uMipmapIndex = 0;
	subImageInfo.uOffset = 0;
	subImageInfo.uSize = result.pData->GetSize();
	subImageInfo.uWidth = width;
	subImageInfo.uHeight = height;
	result.pData->GetSubImageInfo().push_back(subImageInfo);

	return true;
}

static void FlipEndian(void* data, size_t size)
{
	assert(size > 0);
	for(size_t idx = 0; idx < (size >> 1); ++idx)
	{
		((char*)data)[idx] = ((char*)data)[size - 1 - idx];
	}
}

bool KETCCodec::DecodeKTX(const IKDataStreamPtr& stream, KCodecResult& result)
{
	KTXHeader header;
	// Read the KTX header
	stream->Read((char*)&header, sizeof(KTXHeader));

	const uint8_t KTXFileIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
	if (memcmp(KTXFileIdentifier, &header.identifier, sizeof(KTXFileIdentifier)) != 0 )
	{
		return false;
	}

	if (header.endianness == KTX_ENDIAN_REF_REV)
	{
		FlipEndian(&header.glType, sizeof(uint32_t));
	}

	result.uDepth = 1;
	result.uWidth = header.pixelWidth;
	result.uHeight = header.pixelHeight;
	result.uMipmap = header.numberOfMipmapLevels;

	switch(header.glInternalFormat)
	{
	case 0x8D64: // GL_COMPRESSED_RGB8_ETC1
		result.eFormat = IF_ETC1_RGB8;
		break;
	case 0x9274: // GL_COMPRESSED_RGB8_ETC2
		result.eFormat = IF_ETC2_RGB8;
		break;
	case 0x9278:// INTERNAL_RGBA_ETC2
		result.eFormat = IF_ETC2_RGB8A8;
		break;
	case 0x9276: // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
		result.eFormat = IF_ETC2_RGB8A1;
		break;

	case 0x881B: // GL_RGB16F
		result.eFormat = IF_R16G16G16_FLOAT;
		break;
	case 0x881A: // GL_RGBA16F
		result.eFormat = IF_R16G16G16A16_FLOAT;
		break;

	case 0x8815: // GL_RGB32F
		result.eFormat = IF_R32G32G32_FLOAT;
		break;
	case 0x8814: // GL_RGBA32F
		result.eFormat = IF_R32G32G32A32_FLOAT;
		break;

	case 0x8058: // GL_RGBA8
		result.eFormat = IF_R8G8B8A8;
		break;
	case 0x8051: // GL_RGB8
		result.eFormat = IF_R8G8B8;
		break;

	default:
		assert(false && "unknown format");
		break;
	}

	if (header.glType == 0 || header.glFormat == 0)
	{
		result.bCompressed = true;
	}

	size_t numFaces = header.numberOfFaces;
	if (numFaces > 1)
	{
		result.bCubemap = true;
	}

	// Calculate total size from number of mipmaps, faces and size
	size_t imageSize = 0;
	if(KImageHelper::GetByteSize(result.eFormat,
		result.uMipmap,	numFaces,
		result.uWidth, result.uHeight, result.uDepth,
		imageSize))
	{
		result.pData = KImageDataPtr(new KImageData(imageSize));
		// Skip key value data
		stream->Skip(header.bytesOfKeyValueData);

		KSubImageInfoList& subImageInfoList = result.pData->GetSubImageInfo();

		unsigned char* destPtr = result.pData->GetData();

		uint32_t mipOffset = 0;
		size_t width = result.uWidth;
		size_t height = result.uHeight;
		size_t depth = result.uDepth;

		for (uint32_t level = 0; level < header.numberOfMipmapLevels; ++level)
		{
			uint32_t subImageSize = 0;
			stream->Read((char*)&subImageSize, sizeof(uint32_t));

			assert(subImageSize <= imageSize && "impossible to get a subimage bigger than the whole");

			for(uint32_t face = 0; face < numFaces; ++face)
			{
				size_t offset = ((imageSize)/numFaces)*face + mipOffset; // shuffle mip and face
				unsigned char* placePtr = destPtr + offset;

				stream->Read((char*)placePtr, subImageSize);

				KSubImageInfo subImageInfo;
				subImageInfo.uFaceIndex = face;
				subImageInfo.uMipmapIndex = level;
				subImageInfo.uOffset = offset;
				subImageInfo.uSize = subImageSize;
				subImageInfo.uWidth = width;
				subImageInfo.uHeight = height;

				subImageInfoList.push_back(subImageInfo);
			}

			mipOffset += subImageSize;

			if(width > 1) width >>= 1;
			if(height > 1) height >>= 1;
			if(depth > 1) depth >>= 1;
		}

		assert(mipOffset == imageSize && "all subimage size must equal to the whole");

		return true;
	}

	return false;
}

bool KETCCodec::Codec(const char* pszFile, bool forceAlpha, KCodecResult& result)
{
	IKDataStreamPtr stream = GetDataStream(IT_MEMORY);
	if(!stream->Open(pszFile, IM_READ))
	{
		return false;
	}

	stream->Seek(0);
	if (DecodeKTX(stream, result))
	{
		return true;
	}

	stream->Seek(0);
	if (DecodePKM(stream, result))
	{
		return true;
	}

	return false;
}

bool KETCCodec::Init()
{
	IKCodecPtr pCodec = IKCodecPtr(new KETCCodec());
	if(KCodecManager::AddCodec(KTX_EXT, pCodec) && KCodecManager::AddCodec(PKM_EXT, pCodec))
	{
		return true;
	}
	return false;
}

bool KETCCodec::UnInit()
{
	if(KCodecManager::RemoveCodec(KTX_EXT) && KCodecManager::RemoveCodec(PKM_EXT))
	{
		return true;
	}
	return false;
}