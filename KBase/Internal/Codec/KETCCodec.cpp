#include "KETCCodec.h"
#include "Interface/IKFileSystem.h"
#include "Interface/IKLog.h"

// Algorithm Copy From OGRE

#define KTX_EXT "ktx"
#define PKM_EXT "pkm"

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

typedef struct
{
	uint32_t byteOffset; /*!< Offset of item from start of file. */
	uint32_t byteLength; /*!< Number of bytes of data in the item. */
} KTXIndexEntry32;

// 64-bit KTX 2 index entry.
typedef struct
{
	uint64_t byteOffset; /*!< Offset of item from start of file. */
	uint64_t byteLength; /*!< Number of bytes of data in the item. */
} KTXIndexEntry64;

// KTX 2 file header.
// See the KTX 2 specification for descriptions.
typedef struct
{
	uint8_t  identifier[12];
	uint32_t vkFormat;
	uint32_t typeSize;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t layerCount;
	uint32_t faceCount;
	uint32_t levelCount;
	uint32_t supercompressionScheme;
	KTXIndexEntry32 dataFormatDescriptor;
	KTXIndexEntry32 keyValueData;
	KTXIndexEntry64 supercompressionGlobalData;
} KTXHeader2;

typedef union
{
	KTXHeader ktx;
	KTXHeader2 ktx2;
} KTXHeaderUnion;

typedef struct
{
	uint64_t byteOffset;
	uint64_t byteLength;
	uint64_t uncompressedByteLength;
} KTX2LevelIndexEntry;

typedef struct
{
	uint16_t vendorId;
	uint16_t descriptorType;
	uint16_t versionNumber;
	uint16_t descriptorBlockSize;
	uint8_t colorModel;
	uint8_t colorPrimaries;
	uint8_t transferFunction;
	uint8_t flags;
	uint8_t texelBlockDimension[4];
	uint8_t bytesPlane[8];
} KTXBasicDataFormatDescriptorBlock;

typedef struct
{
	uint16_t bitOffset;
	uint8_t bitLength;
	uint8_t channelType : 4;
	uint8_t L : 1;
	uint8_t E : 1;
	uint8_t S : 1;
	uint8_t F : 1;
	uint8_t samplePosition[4];
	uint32_t sampleLower;
	uint32_t sampleUpper;
} KTXBasicDataFormatDescriptorSampleInformation;

static_assert(sizeof(KTXHeader::identifier) == 12, "must be size 12");
static_assert(sizeof(KTXHeader2::identifier) == 12, "must be size 12");
static_assert(sizeof(KTXHeader) == 64, "must be size 64");
static_assert(sizeof(KTXHeader2) == 80, "must be size 80");
static_assert(sizeof(KTXHeaderUnion) == 80, "must be size 80");
static_assert(sizeof(KTXBasicDataFormatDescriptorBlock) == 24, "must be size 24");
static_assert(sizeof(KTXBasicDataFormatDescriptorSampleInformation) == 16, "must be size 16");

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
	result.pData = KImageDataPtr(KNEW KImageData((paddedWidth * paddedHeight) >> 1));

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

bool EnsureDecodeData(KCodecResult& result)
{
	if (result.bCompressed)
	{
		if (KCodec::QueryFormatHardwareDecode(result.eFormat))
		{
			return true;
		}

		if (KCodec::ETC1Format(result.eFormat) || KCodec::ETC2Format(result.eFormat) || KCodec::BCFormat(result.eFormat))
		{
			// TODO
			return false;
		}

		if (KCodec::ASTCFormat(result.eFormat))
		{
			// TODO
			return false;
		}
	}
	else
	{
		return true;
	}
}

static bool DecodeKTX1(const IKDataStreamPtr& stream, KTXHeader& header, KCodecResult& result)
{
	if (header.endianness == KTX_ENDIAN_REF_REV)
	{
		FlipEndian(&header.glType, sizeof(uint32_t));
	}

	result.uDepth = 1;
	result.uWidth = header.pixelWidth;
	result.uHeight = header.pixelHeight;
	result.uMipmap = header.numberOfMipmapLevels;

	switch (header.glInternalFormat)
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
			result.eFormat = IF_R16G16B16_FLOAT;
			break;
		case 0x881A: // GL_RGBA16F
			result.eFormat = IF_R16G16B16A16_FLOAT;
			break;

		case 0x8815: // GL_RGB32F
			result.eFormat = IF_R32G32B32_FLOAT;
			break;
		case 0x8814: // GL_RGBA32F
			result.eFormat = IF_R32G32B32A32_FLOAT;
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

	// Skip key value data
	stream->Skip(header.bytesOfKeyValueData);

	// Calculate total size from number of mipmaps, faces and size
	size_t imageSize = 0;
	if (!KImageHelper::GetByteSize(result.eFormat,
		result.uMipmap, numFaces,
		result.uWidth, result.uHeight, result.uDepth,
		imageSize))
	{
		KG_LOGE(LM_RENDER, "KTX1 get byte size failure!");
		return false;
	}

	result.pData = KImageDataPtr(KNEW KImageData(imageSize));

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

		for (uint32_t face = 0; face < numFaces; ++face)
		{
			size_t offset = ((imageSize) / numFaces) * face + mipOffset; // shuffle mip and face
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

		if (width > 1) width >>= 1;
		if (height > 1) height >>= 1;
		if (depth > 1) depth >>= 1;
	}

	assert(mipOffset * numFaces == imageSize && "all subimage size must equal to the whole");
	return true;
}

static bool CheckKTX2Header(KTXHeader2& header, KCodecResult& result)
{
	/*
		Check texture dimensions. KTX files can store 8 types of textures:
		1D, 2D, 3D, cube, and array variants of these. There is currently
		no extension for 3D array textures in any 3D API.
	 */
	if ((header.pixelWidth == 0) || (header.pixelDepth > 0 && header.pixelHeight == 0))
	{
		return false;
	}

	uint32_t textureDimension = 0;
	if (header.pixelDepth > 0)
	{
		if (header.layerCount > 0)
		{
			/* No 3D array textures yet. */
			return false;
		}
		result.b3DTexture = true;
		textureDimension = 3;
	}
	else
	{
		header.pixelDepth = 1;
		if (header.pixelHeight > 0)
		{
			textureDimension = 2;
		}
		else
		{
			header.pixelHeight = 1;
			textureDimension = 1;
		}
	}

	if (header.faceCount == 6)
	{
		if (textureDimension != 2)
		{
			/* cube map needs 2D faces */
			return false;
		}
		result.bCubemap = true;
	}
	else if (header.faceCount != 1)
	{
		/* numberOfFaces must be either 1 or 6 */
		return false;
	}

	// Check number of mipmap levels
	if (header.levelCount == 0)
	{
		header.levelCount = 1;
	}

	if (header.layerCount == 0)
	{
		header.layerCount = 1;
	}

	// This test works for arrays too because height or depth will be 0.
	uint32_t max_dim = std::max(std::max(header.pixelWidth, header.pixelHeight), header.pixelDepth);
	if (max_dim < (uint32_t)1 << (header.levelCount - 1))
	{
		// Can't have more mip levels than 1 + log2(max(width, height, depth))
		return false;
	}

	result.uWidth = header.pixelWidth;
	result.uHeight = header.pixelHeight;
	result.uDepth = header.pixelDepth;
	result.uMipmap = header.levelCount;

	return true;
}

static bool DecodeKTX2(const IKDataStreamPtr& stream, KTXHeader2& header, KCodecResult& result)
{
	if (!CheckKTX2Header(header, result))
	{
		return false;
	}
	// 暂时不支持数组
	if (header.layerCount > 1)
	{
		return false;
	}
	// 不支持超压缩
	if (header.supercompressionScheme != 0)
	{
		return false;
	}

	std::vector<KTX2LevelIndexEntry> levels;
	levels.resize(header.levelCount);

	if (!stream->Read(levels.data(), sizeof(KTX2LevelIndexEntry) * header.levelCount))
	{
		return false;
	}

	uint64_t totalSize = 0;

	uint64_t firstLevelOffset = levels[header.levelCount - 1].byteOffset;
	// Rebase index to start of data and save file offset.
	for (uint32_t idx = 0; idx < header.levelCount; ++idx)
	{
		levels[idx].byteOffset -= firstLevelOffset;
		totalSize += levels[idx].byteLength;
	}

	uint32_t dfdTotalSize = 0;
	if (!stream->Read(&dfdTotalSize, sizeof(uint32_t)))
	{
		return false;
	}

	assert(header.dataFormatDescriptor.byteLength == dfdTotalSize);
	assert(header.dataFormatDescriptor.byteOffset + header.dataFormatDescriptor.byteLength == header.keyValueData.byteOffset);

	KTXBasicDataFormatDescriptorBlock bdb;
	stream->Read(&bdb, sizeof(KTXBasicDataFormatDescriptorBlock));

	assert(dfdTotalSize - 4 == bdb.descriptorBlockSize);

	uint32_t numSamples = (bdb.descriptorBlockSize - 24) / 16;

	std::vector<KTXBasicDataFormatDescriptorSampleInformation> bdbSamples;
	bdbSamples.resize(numSamples);

	stream->Read(bdbSamples.data(), numSamples * sizeof(KTXBasicDataFormatDescriptorSampleInformation));

	// Skip key value data
	stream->Skip(header.keyValueData.byteLength);

	if (header.supercompressionGlobalData.byteLength > 0)
	{
		assert(header.supercompressionGlobalData.byteOffset == (uint64_t)header.keyValueData.byteOffset + (uint64_t)header.keyValueData.byteLength + (uint64_t)8);
		// Skip 8 padding
		stream->Skip(8);
		// Skip super compression
		stream->Skip(header.supercompressionGlobalData.byteLength);
		return false;
	}

	switch (header.vkFormat)
	{
		case 37: // VK_FORMAT_R8G8B8A8_UNORM
			result.eFormat = IF_R8G8B8A8;
			break;
		case 30: // VK_FORMAT_B8G8R8_UNORM
			result.eFormat = IF_R8G8B8;
			break;
		case 16: // VK_FORMAT_R8G8_UNORM
			result.eFormat = IF_R8G8;
			break;
		case 9: // VK_FORMAT_R8_UNORM
			result.eFormat = IF_R8;
			break;
		case 76: //VK_FORMAT_R16_SFLOAT
			result.eFormat = IF_R16_FLOAT;
			break;
		case 83: // VK_FORMAT_R16G16_SFLOAT
			result.eFormat = IF_R16G16_FLOAT;
			break;
		case 90: // VK_FORMAT_R16G16B16_SFLOAT
			result.eFormat = IF_R16G16B16_FLOAT;
			break;
		case 97: // VK_FORMAT_R16G16B16A16_SFLOAT
			result.eFormat = IF_R16G16B16A16_FLOAT;
			break;
		case 100: // VK_FORMAT_R32_SFLOAT
			result.eFormat = IF_R32_FLOAT;
			break;
		case 103: // VK_FORMAT_R32G32_SFLOAT
			result.eFormat = IF_R32G32_FLOAT;
			break;
		case 106: // VK_FORMAT_R32G32B32_SFLOAT
			result.eFormat = IF_R32G32B32_FLOAT;
			break;
		case 109: // VK_FORMAT_R32G32B32A32_SFLOAT
			result.eFormat = IF_R32G32B32A32_FLOAT;
			break;
		case 74: // VK_FORMAT_R16_UINT
			result.eFormat = IF_R16_UINT;
			break;
		case 98: // VK_FORMAT_R32_UINT
			result.eFormat = IF_R32_UINT;
			break;
		case 147: // VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK
			// result.eFormat = IF_ETC1_RGB8;
			// break;
			result.eFormat = IF_ETC2_RGB8;
			break;
		case 151: // VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK
			result.eFormat = IF_ETC2_RGB8A8;
			break;
		case 149: // VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK
			result.eFormat = IF_ETC2_RGB8A1;
			break;
		case 131: // VK_FORMAT_BC1_RGB_UNORM_BLOCK
			result.eFormat = IF_DXT1;
			break;
		case 137: // VK_FORMAT_BC3_UNORM_BLOCK
			// TODO
			// result.eFormat = IF_DXT2;
			// break;
			// result.eFormat = IF_DXT3;
			// break;
			// result.eFormat = IF_DXT4;
			// break;
			result.eFormat = IF_DXT5;
			break;
		case 139: // VK_FORMAT_BC4_UNORM_BLOCK
			result.eFormat = IF_BC4_UNORM;
			break;
		case 140: // VK_FORMAT_BC4_SNORM_BLOCK
			result.eFormat = IF_BC4_SNORM;
			break;
		case 141: // VK_FORMAT_BC5_UNORM_BLOCK
			result.eFormat = IF_BC5_UNORM;
			break;
		case 142: // VK_FORMAT_BC5_SNORM_BLOCK
			result.eFormat = IF_BC5_SNORM;
			break;
		case 143: // VK_FORMAT_BC6H_UFLOAT_BLOCK
			result.eFormat = IF_BC6H_UF16;
			break;
		case 144: // VK_FORMAT_BC6H_SFLOAT_BLOCK
			result.eFormat = IF_BC6H_SF16;
			break;
		case 145: // VK_FORMAT_BC7_UNORM_BLOCK
			result.eFormat = IF_BC7_UNORM;
			break;
		case 146: // VK_FORMAT_BC7_SRGB_BLOCK
			result.eFormat = IF_BC7_UNORM_SRGB;
			break;

		case 157: // VK_FORMAT_ASTC_4x4_UNORM_BLOCK
			result.eFormat = IF_ASTC_4x4_UNORM;
			break;
		case 158: // VK_FORMAT_ASTC_4x4_SRGB_BLOCK
			result.eFormat = IF_ASTC_4x4_SRGB;
			break;
		case 159: // VK_FORMAT_ASTC_5x4_UNORM_BLOCK
			result.eFormat = IF_ASTC_5x4_UNORM;
			break;
		case 160: // VK_FORMAT_ASTC_5x4_SRGB_BLOCK
			result.eFormat = IF_ASTC_5x4_SRGB;
			break;
		case 161: // VK_FORMAT_ASTC_5x5_UNORM_BLOCK
			result.eFormat = IF_ASTC_5x5_UNORM;
			break;
		case 162: // VK_FORMAT_ASTC_5x5_SRGB_BLOCK
			result.eFormat = IF_ASTC_5x5_SRGB;
			break;
		case 163: // VK_FORMAT_ASTC_6x5_UNORM_BLOCK
			result.eFormat = IF_ASTC_6x5_UNORM;
			break;
		case 164: // VK_FORMAT_ASTC_6x5_SRGB_BLOCK
			result.eFormat = IF_ASTC_6x5_SRGB;
			break;
		case 165: // VK_FORMAT_ASTC_6x6_UNORM_BLOCK
			result.eFormat = IF_ASTC_6x6_UNORM;
			break;
		case 166: // VK_FORMAT_ASTC_6x6_SRGB_BLOCK
			result.eFormat = IF_ASTC_6x6_SRGB;
			break;
		case 167: // VK_FORMAT_ASTC_8x5_UNORM_BLOCK
			result.eFormat = IF_ASTC_8x5_UNORM;
			break;
		case 168: // VK_FORMAT_ASTC_8x5_SRGB_BLOCK
			result.eFormat = IF_ASTC_8x5_SRGB;
			break;
		case 169: // VK_FORMAT_ASTC_8x6_UNORM_BLOCK
			result.eFormat = IF_ASTC_8x6_UNORM;
			break;
		case 170: // VK_FORMAT_ASTC_8x6_SRGB_BLOCK
			result.eFormat = IF_ASTC_8x6_SRGB;
			break;
		case 171: // VK_FORMAT_ASTC_8x8_UNORM_BLOCK
			result.eFormat = IF_ASTC_8x8_UNORM;
			break;
		case 172: // VK_FORMAT_ASTC_8x8_SRGB_BLOCK
			result.eFormat = IF_ASTC_8x8_SRGB;
			break;
		case 173: // VK_FORMAT_ASTC_10x5_UNORM_BLOCK 
			result.eFormat = IF_ASTC_10x5_UNORM;
			break;
		case 174: // VK_FORMAT_ASTC_10x5_SRGB_BLOCK
			result.eFormat = IF_ASTC_10x5_SRGB;
			break;
		case 175: // VK_FORMAT_ASTC_10x6_UNORM_BLOCK 
			result.eFormat = IF_ASTC_10x6_UNORM;
			break;
		case 176: // VK_FORMAT_ASTC_10x6_SRGB_BLOCK
			result.eFormat = IF_ASTC_10x6_SRGB;
			break;
		case 177: // VK_FORMAT_ASTC_10x8_UNORM_BLOCK 
			result.eFormat = IF_ASTC_10x8_UNORM;
			break;
		case 178: // VK_FORMAT_ASTC_10x8_SRGB_BLOCK
			result.eFormat = IF_ASTC_10x8_SRGB;
			break;
		case 179: // VK_FORMAT_ASTC_10x10_UNORM_BLOCK
			result.eFormat = IF_ASTC_10x10_UNORM;
			break;
		case 180: // VK_FORMAT_ASTC_10x10_SRGB_BLOCK 
			result.eFormat = IF_ASTC_10x10_SRGB;
			break;
		case 181: // VK_FORMAT_ASTC_12x10_UNORM_BLOCK
			result.eFormat = IF_ASTC_12x10_UNORM;
			break;
		case 182: // VK_FORMAT_ASTC_12x10_SRGB_BLOCK 
			result.eFormat = IF_ASTC_12x10_SRGB;
			break;
		case 183: // VK_FORMAT_ASTC_12x12_UNORM_BLOCK
			result.eFormat = IF_ASTC_12x12_UNORM;
			break;
		case 184: // VK_FORMAT_ASTC_12x12_SRGB_BLOCK 	
			result.eFormat = IF_ASTC_12x12_SRGB;
			break;
	}

	KImageHelper::GetIsCompress(result.eFormat, result.bCompressed);
	
	uint32_t numLayers = header.layerCount;
	uint32_t numFaces = header.faceCount;
	uint32_t mipOffset = 0;

	// Calculate total size from number of mipmaps, faces and size
	size_t imageSize = 0;
	if (!KImageHelper::GetByteSize(result.eFormat,
		result.uMipmap, numFaces,
		result.uWidth, result.uHeight, result.uDepth,
		imageSize))
	{
		KG_LOGE(LM_RENDER, "KTX2 get byte size failure!");
		return false;
	}

	assert(imageSize == totalSize);

	result.pData = KImageDataPtr(KNEW KImageData(imageSize));

	KSubImageInfoList& subImageInfoList = result.pData->GetSubImageInfo();
	unsigned char* destPtr = result.pData->GetData();

	size_t width = result.uWidth;
	size_t height = result.uHeight;
	size_t depth = result.uDepth;

	size_t pos = stream->Tell();

	for (uint32_t level = 0; level < header.levelCount; ++level)
	{
		stream->Seek((long)(pos + levels[level].byteOffset));

		uint32_t subImageSize = (uint32_t)levels[level].byteLength;
		assert(subImageSize <= imageSize && "impossible to get a subimage bigger than the whole");

		subImageSize /= numFaces * numLayers;

		for (uint32_t face = 0; face < numFaces; ++face)
		{
			size_t offset = ((imageSize) / numFaces) * face + mipOffset; // shuffle mip and face
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

		if (width > 1) width >>= 1;
		if (height > 1) height >>= 1;
		if (depth > 1) depth >>= 1;

		stream->Seek((long)pos);
	}

	assert(mipOffset * numFaces == imageSize && "all subimage size must equal to the whole");
	return true;
}

bool KETCCodec::DecodeKTX(const IKDataStreamPtr& stream, KCodecResult& result)
{
	KTXHeaderUnion header;
	// Read the KTX header
	stream->Read((char*)&header.ktx.identifier, sizeof(header.ktx.identifier));

	enum { KTX1, KTX2, KTX_UNKNOWN };

	uint8_t ktxType = KTX_UNKNOWN;

	const uint8_t KTXFileIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
	if (memcmp(KTXFileIdentifier, &header.ktx.identifier, sizeof(KTXFileIdentifier)) == 0)
	{
		ktxType = KTX1;
	}

	const uint8_t KTX2FileIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
	if (memcmp(KTX2FileIdentifier, &header.ktx2.identifier, sizeof(KTXFileIdentifier)) == 0)
	{
		ktxType = KTX2;
	}

	if (ktxType == KTX1)
	{
		stream->Read((char*)&header.ktx.endianness, sizeof(KTXHeader) - sizeof(header.ktx.identifier));
		return DecodeKTX1(stream, header.ktx, result);
	}
	else if (ktxType == KTX2)
	{
		stream->Read((char*)&header.ktx2.vkFormat, sizeof(KTXHeader2) - sizeof(header.ktx2.identifier));
		return DecodeKTX2(stream, header.ktx2, result);
	}
	else
	{
		return false;
	}
}

bool KETCCodec::CodecImpl(IKDataStreamPtr stream, bool forceAlpha, KCodecResult& result)
{
	enum FileFormat
	{
		KTX,
		PKM,
		UNKNOWN
	};

	FileFormat format = UNKNOWN;

	if (format == UNKNOWN)
	{
		stream->Seek(0);
		if (DecodeKTX(stream, result))
		{
			format = KTX;
		}
	}

	if (format == UNKNOWN)
	{
		stream->Seek(0);
		if (DecodePKM(stream, result))
		{
			format = PKM;
		}
	}

	if (format == UNKNOWN)
	{
		KG_LOGE(LM_RENDER, "This is not a ETC file!");
		return false;
	}
	else
	{
		if (!EnsureDecodeData(result))
		{
			KG_LOGE(LM_RENDER, "No hareware decode suppport found!");
			return false;
		}
		return true;
	}
}

bool KETCCodec::Save(const KCodecResult& source, const char* pszFile)
{
	return false;
}

bool KETCCodec::Init()
{
	IKCodecPtr pCodec = IKCodecPtr(KNEW KETCCodec());
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