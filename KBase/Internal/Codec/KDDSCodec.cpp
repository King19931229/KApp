#include "KDDSCodec.h"
#include "Interface/IKFileSystem.h"
#include "Interface/IKLog.h"
#include <algorithm>

// Algorithm Copy From OGRE

#define DDS_EXT "dds"

// Internal DDS structure definitions
#define FOURCC(c0, c1, c2, c3) (c0 | (c1 << 8) | (c2 << 16) | (c3 << 24))

#pragma pack(push,1)

struct ColourValue
{
	float r;
	float g;
	float b;
	float a;

	ColourValue()
	{
		r = 0.0f;
		g = 0.0f;
		b = 0.0f;
		a = 0.0f;
	}

	ColourValue(float _r, float _g, float _b, float _a)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}

	inline ColourValue operator+ (const ColourValue &rkVector) const
	{
		ColourValue result;
		result.r = r + rkVector.r;
		result.g = g + rkVector.g;
		result.b = b + rkVector.b;
		result.a = a + rkVector.a;
		return result;
	}

	inline ColourValue operator* (const float fScalar) const
	{
		ColourValue result;
		result.r = fScalar * r;
		result.g = fScalar * g;
		result.b = fScalar * b;
		result.a = fScalar * a;
		return result;
	}
};

// Nested structure
struct DDSPixelFormat
{
	uint32_t size;
	uint32_t flags;
	uint32_t fourCC;
	uint32_t rgbBits;
	uint32_t redMask;
	uint32_t greenMask;
	uint32_t blueMask;
	uint32_t alphaMask;
};

// Nested structure
struct DDSCaps
{
	uint32_t caps1;
	uint32_t caps2;
	uint32_t reserved[2];
};

// Main header, note preceded by 'DDS '
struct DDSHeader
{
	uint32_t size;
	uint32_t flags;
	uint32_t height;
	uint32_t width;
	uint32_t sizeOrPitch;
	uint32_t depth;
	uint32_t mipMapCount;
	uint32_t reserved1[11];
	DDSPixelFormat pixelFormat;
	DDSCaps caps;
	uint32_t reserved2;
};

// External header for DXT10
struct DDSHeaderDXT10
{
	uint32_t dxgiFormat;
	uint32_t resourceDimension;
	uint32_t miscFlag; // see DDS_RESOURCE_MISC_FLAG
	uint32_t arraySize;
	uint32_t miscFlags2; // see DDS_MISC_FLAGS2
};

// An 8-byte DXT colour block, represents a 4x4 texel area. Used by all DXT formats
struct DXTColourBlock
{
	// 2 colour ranges
	uint16_t colour_0;
	uint16_t colour_1;
	// 16 2-bit indexes, each byte here is one row
	uint8_t indexRow[4];
};

// An 8-byte DXT explicit alpha block, represents a 4x4 texel area. Used by DXT2/3
struct DXTExplicitAlphaBlock
{
	// 16 4-bit values, each 16-bit value is one row
	uint16_t alphaRow[4];
};

// An 8-byte DXT interpolated alpha block, represents a 4x4 texel area. Used by DXT4/5
struct DXTInterpolatedAlphaBlock
{
	// 2 alpha ranges
	uint8_t alpha_0;
	uint8_t alpha_1;
	// 16 3-bit indexes. Unfortunately 3 bits doesn't map too well to row bytes
	// so just stored raw
	uint8_t indexes[6];
};

#pragma pack (pop)

static const uint32_t DDS_MAGIC = FOURCC('D', 'D', 'S', ' ');
static const uint32_t DDS_PIXELFORMAT_SIZE = 8 * sizeof(uint32_t);
static const uint32_t DDS_CAPS_SIZE = 4 * sizeof(uint32_t);
static const uint32_t DDS_HEADER_SIZE = 19 * sizeof(uint32_t) + DDS_PIXELFORMAT_SIZE + DDS_CAPS_SIZE;

static const uint32_t DDSD_CAPS = 0x00000001;
static const uint32_t DDSD_HEIGHT = 0x00000002;
static const uint32_t DDSD_WIDTH = 0x00000004;
static const uint32_t DDSD_PITCH = 0x00000008;
static const uint32_t DDSD_PIXELFORMAT = 0x00001000;
static const uint32_t DDSD_MIPMAPCOUNT = 0x00020000;
static const uint32_t DDSD_LINEARSIZE = 0x00080000;
static const uint32_t DDSD_DEPTH = 0x00800000;
static const uint32_t DDPF_ALPHAPIXELS = 0x00000001;
static const uint32_t DDPF_FOURCC = 0x00000004;
static const uint32_t DDPF_RGB = 0x00000040;
static const uint32_t DDSCAPS_COMPLEX = 0x00000008;
static const uint32_t DDSCAPS_TEXTURE = 0x00001000;
static const uint32_t DDSCAPS_MIPMAP = 0x00400000;
static const uint32_t DDSCAPS2_CUBEMAP = 0x00000200;
static const uint32_t DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400;
static const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800;
static const uint32_t DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000;
static const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000;
static const uint32_t DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000;
static const uint32_t DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000;
static const uint32_t DDSCAPS2_VOLUME = 0x00200000;

// Special FourCC codes
static const uint32_t D3DFMT_DXT1 = FOURCC('D', 'X', 'T', '1');
static const uint32_t D3DFMT_DXT2 = FOURCC('D', 'X', 'T', '2');
static const uint32_t D3DFMT_DXT3 = FOURCC('D', 'X', 'T', '3');
static const uint32_t D3DFMT_DXT4 = FOURCC('D', 'X', 'T', '4');
static const uint32_t D3DFMT_DXT5 = FOURCC('D', 'X', 'T', '5');
static const uint32_t D3DFMT_DX10 = FOURCC('D', 'X', '1', '0');
static const uint32_t D3DFMT_R16F = 111;
static const uint32_t D3DFMT_G16R16F = 112;
static const uint32_t D3DFMT_A16B16G16R16F = 113;
static const uint32_t D3DFMT_R32F = 114;
static const uint32_t D3DFMT_G32R32F = 115;
static const uint32_t D3DFMT_A32B32G32R32F = 116;

static const uint32_t DDS_DXT10_DIMENSION_TEXTURE1D = 2;
static const uint32_t DDS_DXT10_DIMENSION_TEXTURE2D = 3;
static const uint32_t DDS_DXT10_DIMENSION_TEXTURE3D = 4;
static const uint32_t DDS_DXT10_MISC_TEXTURE_CUBE = 4;

enum DXGI_FORMAT
{
	DXGI_FORMAT_UNKNOWN = 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
	DXGI_FORMAT_R32G32B32A32_UINT = 3,
	DXGI_FORMAT_R32G32B32A32_SINT = 4,
	DXGI_FORMAT_R32G32B32_TYPELESS = 5,
	DXGI_FORMAT_R32G32B32_FLOAT = 6,
	DXGI_FORMAT_R32G32B32_UINT = 7,
	DXGI_FORMAT_R32G32B32_SINT = 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
	DXGI_FORMAT_R16G16B16A16_UNORM = 11,
	DXGI_FORMAT_R16G16B16A16_UINT = 12,
	DXGI_FORMAT_R16G16B16A16_SNORM = 13,
	DXGI_FORMAT_R16G16B16A16_SINT = 14,
	DXGI_FORMAT_R32G32_TYPELESS = 15,
	DXGI_FORMAT_R32G32_FLOAT = 16,
	DXGI_FORMAT_R32G32_UINT = 17,
	DXGI_FORMAT_R32G32_SINT = 18,
	DXGI_FORMAT_R32G8X24_TYPELESS = 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
	DXGI_FORMAT_R10G10B10A2_UNORM = 24,
	DXGI_FORMAT_R10G10B10A2_UINT = 25,
	DXGI_FORMAT_R11G11B10_FLOAT = 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
	DXGI_FORMAT_R8G8B8A8_UNORM = 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
	DXGI_FORMAT_R8G8B8A8_UINT = 30,
	DXGI_FORMAT_R8G8B8A8_SNORM = 31,
	DXGI_FORMAT_R8G8B8A8_SINT = 32,
	DXGI_FORMAT_R16G16_TYPELESS = 33,
	DXGI_FORMAT_R16G16_FLOAT = 34,
	DXGI_FORMAT_R16G16_UNORM = 35,
	DXGI_FORMAT_R16G16_UINT = 36,
	DXGI_FORMAT_R16G16_SNORM = 37,
	DXGI_FORMAT_R16G16_SINT = 38,
	DXGI_FORMAT_R32_TYPELESS = 39,
	DXGI_FORMAT_D32_FLOAT = 40,
	DXGI_FORMAT_R32_FLOAT = 41,
	DXGI_FORMAT_R32_UINT = 42,
	DXGI_FORMAT_R32_SINT = 43,
	DXGI_FORMAT_R24G8_TYPELESS = 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
	DXGI_FORMAT_R8G8_TYPELESS = 48,
	DXGI_FORMAT_R8G8_UNORM = 49,
	DXGI_FORMAT_R8G8_UINT = 50,
	DXGI_FORMAT_R8G8_SNORM = 51,
	DXGI_FORMAT_R8G8_SINT = 52,
	DXGI_FORMAT_R16_TYPELESS = 53,
	DXGI_FORMAT_R16_FLOAT = 54,
	DXGI_FORMAT_D16_UNORM = 55,
	DXGI_FORMAT_R16_UNORM = 56,
	DXGI_FORMAT_R16_UINT = 57,
	DXGI_FORMAT_R16_SNORM = 58,
	DXGI_FORMAT_R16_SINT = 59,
	DXGI_FORMAT_R8_TYPELESS = 60,
	DXGI_FORMAT_R8_UNORM = 61,
	DXGI_FORMAT_R8_UINT = 62,
	DXGI_FORMAT_R8_SNORM = 63,
	DXGI_FORMAT_R8_SINT = 64,
	DXGI_FORMAT_A8_UNORM = 65,
	DXGI_FORMAT_R1_UNORM = 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
	DXGI_FORMAT_BC1_TYPELESS = 70,
	DXGI_FORMAT_BC1_UNORM = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB = 72,
	DXGI_FORMAT_BC2_TYPELESS = 73,
	DXGI_FORMAT_BC2_UNORM = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB = 75,
	DXGI_FORMAT_BC3_TYPELESS = 76,
	DXGI_FORMAT_BC3_UNORM = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB = 78,
	DXGI_FORMAT_BC4_TYPELESS = 79,
	DXGI_FORMAT_BC4_UNORM = 80,
	DXGI_FORMAT_BC4_SNORM = 81,
	DXGI_FORMAT_BC5_TYPELESS = 82,
	DXGI_FORMAT_BC5_UNORM = 83,
	DXGI_FORMAT_BC5_SNORM = 84,
	DXGI_FORMAT_B5G6R5_UNORM = 85,
	DXGI_FORMAT_B5G5R5A1_UNORM = 86,
	DXGI_FORMAT_B8G8R8A8_UNORM = 87,
	DXGI_FORMAT_B8G8R8X8_UNORM = 88,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
	DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
	DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
	DXGI_FORMAT_BC6H_TYPELESS = 94,
	DXGI_FORMAT_BC6H_UF16 = 95,
	DXGI_FORMAT_BC6H_SF16 = 96,
	DXGI_FORMAT_BC7_TYPELESS = 97,
	DXGI_FORMAT_BC7_UNORM = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB = 99,
	DXGI_FORMAT_AYUV = 100,
	DXGI_FORMAT_Y410 = 101,
	DXGI_FORMAT_Y416 = 102,
	DXGI_FORMAT_NV12 = 103,
	DXGI_FORMAT_P010 = 104,
	DXGI_FORMAT_P016 = 105,
	DXGI_FORMAT_420_OPAQUE = 106,
	DXGI_FORMAT_YUY2 = 107,
	DXGI_FORMAT_Y210 = 108,
	DXGI_FORMAT_Y216 = 109,
	DXGI_FORMAT_NV11 = 110,
	DXGI_FORMAT_AI44 = 111,
	DXGI_FORMAT_IA44 = 112,
	DXGI_FORMAT_P8 = 113,
	DXGI_FORMAT_A8P8 = 114,
	DXGI_FORMAT_B4G4R4A4_UNORM = 115,
	DXGI_FORMAT_FORCE_UINT = 0xffffffff
};

KDDSCodec::KDDSCodec()
{}

KDDSCodec::~KDDSCodec()
{}

bool KDDSCodec::Init()
{
	IKCodecPtr pCodec = IKCodecPtr(KNEW KDDSCodec());
	if (KCodecManager::AddCodec(DDS_EXT, pCodec))
	{
		return true;
	}
	return false;
}

bool KDDSCodec::UnInit()
{
	if (KCodecManager::RemoveCodec(DDS_EXT))
	{
		return true;
	}
	return false;
}

ImageFormat KDDSCodec::ConvertFourCCFormat(uint32_t fourcc) const
{
	// convert dxt pixel format
	switch (fourcc)
	{
	case FOURCC('D', 'X', 'T', '1'):
		return IF_DXT1;
	case FOURCC('D', 'X', 'T', '2'):
		return IF_DXT2;
	case FOURCC('D', 'X', 'T', '3'):
		return IF_DXT3;
	case FOURCC('D', 'X', 'T', '4'):
		return IF_DXT4;
	case FOURCC('D', 'X', 'T', '5'):
		return IF_DXT5;
	case FOURCC('A', 'T', 'I', '1'):
	case FOURCC('B', 'C', '4', 'U'):
		return IF_BC4_UNORM;
	case FOURCC('B', 'C', '4', 'S'):
		return IF_BC4_SNORM;
	case FOURCC('A', 'T', 'I', '2'):
	case FOURCC('B', 'C', '5', 'U'):
		return IF_BC5_UNORM;
	case FOURCC('B', 'C', '5', 'S'):
		return IF_BC5_SNORM;
	case D3DFMT_R16F:
		return IF_R16_FLOAT;
	case D3DFMT_G16R16F:
		return IF_R16G16_FLOAT;
	case D3DFMT_A16B16G16R16F:
		return IF_R16G16B16A16_FLOAT;
	case D3DFMT_R32F:
		return IF_R32_FLOAT;
	case D3DFMT_G32R32F:
		return IF_R32G32_FLOAT;
	case D3DFMT_A32B32G32R32F:
		return IF_R32G32B32A32_FLOAT;
		// We could support 3Dc here, but only ATI cards support it, not nVidia
	default:
		assert(false && "unknown ccformat");
		return IF_INVALID;
	};
}

ImageFormat KDDSCodec::SearchImageFormat(uint32_t rgbBits, uint32_t rMask, uint32_t gMask, uint32_t bMask, uint32_t aMask) const
{
	return IF_INVALID;
}

void KDDSCodec::UnpackR5G6B5Colour(const void* src, ColourValue *pCol) const
{
	auto FIX_TO_FLOAT = [](int32_t value, int32_t bits)->float
	{
		return (float)value / (float)((1 << bits) - 1);
	};

	uint32_t intValue =
#ifdef NEED_FLIP_ENDIAN
	((uint32_t)((uint8_t *)src)[0] << 16) |
		((uint32_t)((uint8_t *)src)[1] << 8) |
		((uint32_t)((uint8_t *)src)[2]);
#else
		((uint32_t)((uint8_t *)src)[0]) |
		((uint32_t)((uint8_t *)src)[1] << 8) |
		((uint32_t)((uint8_t *)src)[2] << 16);
#endif
	pCol->r = FIX_TO_FLOAT((intValue & 0xF800) >> 11, 5);
	pCol->g = FIX_TO_FLOAT((intValue & 0x07E0) >> 5, 6);
	pCol->b = FIX_TO_FLOAT((intValue & 0x001F) >> 0, 5);
	pCol->a = 1.0f;
}

void KDDSCodec::UnpackDXTColour(ImageFormat format, const DXTColourBlock &block,
	ColourValue *pCol) const
{
	// Note - we assume all values have already been endian swapped

	// Colour lookup table
	ColourValue derivedColours[4];

	if (format == IF_DXT1 && block.colour_0 <= block.colour_1)
	{
		// 1-bit alpha
		UnpackR5G6B5Colour(&(block.colour_0), &derivedColours[0]);
		UnpackR5G6B5Colour(&(block.colour_1), &derivedColours[1]);
		// one intermediate colour, half way between the other two
		derivedColours[2] = (derivedColours[0] + derivedColours[1]) * 0.5f;
		// transparent colour
		derivedColours[3] = { 0.0f, 0.0f, 0.0f, 0.0f };
	}
	else
	{
		UnpackR5G6B5Colour(&(block.colour_0), &derivedColours[0]);
		UnpackR5G6B5Colour(&(block.colour_1), &derivedColours[1]);
		// first interpolated colour, 1/3 of the way along
		derivedColours[2] = (derivedColours[0] * 2 + derivedColours[1]) * 0.333f;
		// second interpolated colour, 2/3 of the way along
		derivedColours[3] = (derivedColours[0] + derivedColours[1] * 2) * 0.333f;
	}

	// Process 4x4 block of texels
	for (size_t row = 0; row < 4; ++row)
	{
		for (size_t x = 0; x < 4; ++x)
		{
			// LSB come first
			uint8_t colIdx = static_cast<uint8_t>(block.indexRow[row] >> (x * 2) & 0x3);
			if (format == IF_DXT1)
			{
				// Overwrite entire colour
				pCol[(row * 4) + x] = derivedColours[colIdx];
			}
			else
			{
				// alpha has already been read (alpha precedes colour)
				ColourValue &col = pCol[(row * 4) + x];
				col.r = derivedColours[colIdx].r;
				col.g = derivedColours[colIdx].g;
				col.b = derivedColours[colIdx].b;
			}
		}
	}
}

void KDDSCodec::UnpackDXTAlpha(const DXTExplicitAlphaBlock &block, ColourValue *pCol) const
{
	// Note - we assume all values have already been endian swapped

	// This is an explicit alpha block, 4 bits per pixel, LSB first
	for (size_t row = 0; row < 4; ++row)
	{
		for (size_t x = 0; x < 4; ++x)
		{
			// Shift and mask off to 4 bits
			uint8_t val = static_cast<uint8_t>(block.alphaRow[row] >> (x * 4) & 0xF);
			// Convert to [0,1]
			pCol->a = (float)val / (float)0xF;
			pCol++;
		}
	}
}

void KDDSCodec::UnpackDXTAlpha(const DXTInterpolatedAlphaBlock &block, ColourValue *pCol) const
{
	// 8 derived alpha values to be indexed
	float derivedAlphas[8];

	// Explicit extremes
	derivedAlphas[0] = block.alpha_0 / (float)0xFF;
	derivedAlphas[1] = block.alpha_1 / (float)0xFF;

	if (block.alpha_0 <= block.alpha_1)
	{
		// 4 interpolated alphas, plus zero and one
		// full range including extremes at [0] and [5]
		// we want to fill in [1] through [4] at weights ranging
		// from 1/5 to 4/5
		float denom = 1.0f / 5.0f;
		for (size_t i = 0; i < 4; ++i)
		{
			float factor0 = (4 - i) * denom;
			float factor1 = (i + 1) * denom;
			derivedAlphas[i + 2] =
				(factor0 * block.alpha_0) + (factor1 * block.alpha_1);
		}
		derivedAlphas[6] = 0.0f;
		derivedAlphas[7] = 1.0f;
	}
	else
	{
		// 6 interpolated alphas
		// full range including extremes at [0] and [7]
		// we want to fill in [1] through [6] at weights ranging
		// from 1/7 to 6/7
		float denom = 1.0f / 7.0f;
		for (size_t i = 0; i < 6; ++i)
		{
			float factor0 = (6 - i) * denom;
			float factor1 = (i + 1) * denom;
			derivedAlphas[i + 2] =
				(factor0 * block.alpha_0) + (factor1 * block.alpha_1);
		}
	}

	// Ok, now we've built the reference values, process the indexes
	for (size_t i = 0; i < 16; ++i)
	{
		size_t baseByte = (i * 3) / 8;
		size_t baseBit = (i * 3) % 8;
		uint8_t bits = static_cast<uint8_t>(block.indexes[baseByte] >> baseBit & 0x7);
		// do we need to stitch in next byte too?
		if (baseBit > 5)
		{
			uint8_t extraBits = static_cast<uint8_t>(
				(block.indexes[baseByte + 1] << (8 - baseBit)) & 0xFF);
			bits |= extraBits & 0x7;
		}
		pCol[i].a = derivedAlphas[bits];
	}
}

bool KDDSCodec::Codec(const char* pszFile, bool forceAlpha, KCodecResult& result)
{
	IKDataStreamPtr stream = nullptr;

	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
	if (!system || !system->Open(pszFile, IT_FILEHANDLE, stream))
	{
		system = KFileSystem::Manager->GetFileSystem(FSD_BACKUP);
		if (!system || !system->Open(pszFile, IT_FILEHANDLE, stream))
		{
			return false;
		}
	}

	// Read 4 character code
	uint32_t fileType;
	stream->Read(&fileType, sizeof(uint32_t));
	FlipEndian(&fileType, sizeof(uint32_t), 1);

	if (FOURCC('D', 'D', 'S', ' ') != fileType)
	{
		KG_LOGE(LM_RENDER, "This is not a DDS file!");
		return false;
	}

	// Read header in full
	DDSHeader header;
	stream->Read(&header, sizeof(DDSHeader));

	// Endian flip if required, all 32-bit values
	FlipEndian(&header, 4, sizeof(DDSHeader) / 4);

	// Check some sizes
	if (header.size != DDS_HEADER_SIZE)
	{
		KG_LOGE(LM_RENDER, "DDS header size mismatch!");
		return false;
	}
	if (header.pixelFormat.size != DDS_PIXELFORMAT_SIZE)
	{
		KG_LOGE(LM_RENDER, "DDS header size mismatch!");
		return false;
	}

	result.uDepth = 1; // (deal with volume later)
	result.uWidth = header.width;
	result.uHeight = header.height;
	uint16_t numFaces = 1; // assume one face until we know otherwise

	if (header.caps.caps1 & DDSCAPS_MIPMAP)
	{
		result.uMipmap = header.mipMapCount;
	}
	else
	{
		result.uMipmap = 1;
	}

	result.bCompressed = false;
	result.bCubemap = false;

	bool decompressDXT = false;
	// Figure out basic image type
	if (header.caps.caps2 & DDSCAPS2_CUBEMAP)
	{
		result.bCubemap = true;
		numFaces = 6;
	}
	else if (header.caps.caps2 & DDSCAPS2_VOLUME)
	{
		result.b3DTexture = true;
		result.uDepth = header.depth;
	}

	ImageFormat sourceFormat = IF_INVALID;

	if (header.pixelFormat.flags & DDPF_FOURCC)
	{
		if (header.pixelFormat.fourCC == FOURCC('D', 'X', '1', '0'))
		{
			DDSHeaderDXT10 headerDXT10;

			stream->Read(&headerDXT10, sizeof(DDSHeaderDXT10));
			FlipEndian(&headerDXT10, 4, sizeof(DDSHeaderDXT10) / 4);

			header.flags = headerDXT10.miscFlag;

			if (headerDXT10.arraySize != 1)
			{
				KG_LOGE(LM_RENDER, "DDS DXT10header arraySize mismatch!");
				return false;
			}

			switch (headerDXT10.resourceDimension)
			{
			case DDS_DXT10_DIMENSION_TEXTURE1D:
				if ((header.flags & DDSD_HEIGHT) && header.height != 1)
				{
					KG_LOGE(LM_RENDER, "DDS DXT10header height mismatch!");
					return false;
				}
				result.uWidth = header.width;
				result.uHeight = 1;
				result.uDepth = 1;
				break;
			case DDS_DXT10_DIMENSION_TEXTURE2D:
				if (headerDXT10.miscFlag & DDS_DXT10_MISC_TEXTURE_CUBE)
				{
					result.bCubemap = true;
					numFaces = 6;
				}
				result.uWidth = header.width;
				result.uHeight = header.height;
				result.uDepth = 1;
				break;
			case DDS_DXT10_DIMENSION_TEXTURE3D:
				if (!(header.flags & DDSD_DEPTH))
				{
					KG_LOGE(LM_RENDER, "DDS DXT10header depth mismatch!");
					return false;
				}
				result.uWidth = header.width;
				result.uHeight = header.height;
				result.uDepth = header.depth;
				break;
			}

			switch (headerDXT10.dxgiFormat)
			{
			case DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				sourceFormat = IF_DXT1;
				break;

			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				sourceFormat = IF_DXT3;
				break;

			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				sourceFormat = IF_DXT5;
				break;

			case DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT_BC4_UNORM:
				sourceFormat = IF_BC4_UNORM;
				break;

			case DXGI_FORMAT_BC4_SNORM:
				sourceFormat = IF_BC5_SNORM;
				break;

			case DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT_BC5_UNORM:
				sourceFormat = IF_BC5_UNORM;
				break;

			case DXGI_FORMAT_BC5_SNORM:
				sourceFormat = IF_BC5_SNORM;
				break;

			case DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT_BC6H_UF16:
				sourceFormat = IF_BC6H_UF16;
				break;

			case DXGI_FORMAT_BC6H_SF16:
				sourceFormat = IF_BC6H_SF16;
				break;

			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
				sourceFormat = IF_BC7_UNORM;
				break;

			case DXGI_FORMAT_BC7_UNORM_SRGB:
				sourceFormat = IF_BC7_UNORM_SRGB;
				break;

			case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
				sourceFormat = IF_R16G16B16A16_FLOAT;
				break;

			case DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT_R32G32_FLOAT:
				sourceFormat = IF_R32G32_FLOAT;
				break;

			case DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT_R16G16_FLOAT:
				sourceFormat = IF_R16G16_FLOAT;
				break;

			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_R32_FLOAT:
				sourceFormat = IF_R32_FLOAT;
				break;

			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_R16_FLOAT:
				sourceFormat = IF_R16_FLOAT;
				break;

			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				sourceFormat = IF_R8G8B8A8;
				break;

			default:
				KG_LOGE(LM_RENDER, "The DXGI format is not supported!");
				return false;
			}
		}
		else
		{
			sourceFormat = ConvertFourCCFormat(header.pixelFormat.fourCC);
		}
	}
	else
	{
		sourceFormat = SearchImageFormat(header.pixelFormat.rgbBits,
			header.pixelFormat.redMask, header.pixelFormat.greenMask,
			header.pixelFormat.blueMask,
			header.pixelFormat.flags & DDPF_ALPHAPIXELS ?
			header.pixelFormat.alphaMask : 0);
	}

	bool isCompress = false;
	KImageHelper::GetIsCompress(sourceFormat, isCompress);
	if (isCompress)
	{
		if (!KCodec::BCHardwareCodec || result.uMipmap == 1/* || result.b3DTexture*/)
		{
			// We'll need to decompress
			decompressDXT = true;
			// Convert format
			switch (sourceFormat)
			{
			case IF_DXT1:
				// source can be either 565 or 5551 depending on whether alpha present
				// unfortunately you have to read a block to figure out which
				// Note that we upgrade to 32-bit pixel formats here, even
				// though the source is 16-bit; this is because the interpolated
				// values will benefit from the 32-bit results, and the source
				// from which the 16-bit samples are calculated may have been
				// 32-bit so can benefit from this.
				DXTColourBlock block;
				stream->Read(&block, sizeof(DXTColourBlock));
				FlipEndian(&(block.colour_0), sizeof(uint16_t), 1);
				FlipEndian(&(block.colour_1), sizeof(uint16_t), 1);
				// skip back since we'll need to read this again
				stream->Skip(0 - (long)sizeof(DXTColourBlock));
				// colour_0 <= colour_1 means transparency in DXT1
				if (block.colour_0 <= block.colour_1)
				{
					result.eFormat = IF_R8G8B8A8;
				}
				else
				{
					result.eFormat = forceAlpha ? IF_R8G8B8A8 : IF_R8G8B8;
				}
				result.bCompressed = false;
				break;
			case IF_DXT2:
			case IF_DXT3:
			case IF_DXT4:
			case IF_DXT5:
				// full alpha present, formats vary only in encoding
				result.eFormat = IF_R8G8B8A8;
				result.bCompressed = false;
				break;
			default:
				// unable to decompress
				decompressDXT = false;
				result.bCompressed = true;
				break;
			}
		}
		else
		{
			// Use original format
			result.eFormat = sourceFormat;
			// Keep DXT data compressed
			result.bCompressed = true;
		}
	}
	else // not compressed
	{
		// Don't test against DDPF_RGB since greyscale DDS doesn't set this
		// just derive any other kind of format
		result.eFormat = sourceFormat;
		result.bCompressed = false;
	}

	if (result.bCompressed && !KCodec::BCHardwareCodec)
	{
		KG_LOGE(LM_RENDER, "The DXGI format is compressed but hardware codec is not supported!");
		return false;
	}

	size_t imageSize = 0;
	if (!KImageHelper::GetByteSize(result.eFormat,
		result.uMipmap,
		numFaces,
		result.uWidth,
		result.uHeight,
		result.uDepth,
		imageSize))
	{
		KG_LOGE(LM_RENDER, "DDS get byte size failure!");
		return false;
	}

	result.pData = KImageDataPtr(KNEW KImageData(imageSize));

	// Now deal with the data
	void *destPtr = result.pData->GetData();

	KSubImageInfoList& subImageInfoList = result.pData->GetSubImageInfo();

	// all mips for a face, then each face
	for (uint16_t face = 0; face < numFaces; ++face)
	{
		size_t width = result.uWidth;
		size_t height = result.uHeight;
		size_t depth = result.uDepth;

		for (size_t mip = 0; mip < result.uMipmap; ++mip)
		{
			void *srcData = destPtr;

			if (isCompress)
			{
				// Compressed data
				if (decompressDXT)
				{
					// Get the byte size of the dest format element
					size_t elementByteSize = 0;
					ASSERT_RESULT(KImageHelper::GetElementByteSize(result.eFormat, elementByteSize));
					size_t dstPitch = width * elementByteSize;

					DXTColourBlock col;
					DXTInterpolatedAlphaBlock iAlpha;
					DXTExplicitAlphaBlock eAlpha;
					// 4x4 block of decompressed colour
					ColourValue tempColours[16];
					size_t destBpp = elementByteSize;
					size_t sx = std::min(width, (size_t)4);
					size_t sy = std::min(height, (size_t)4);
					size_t destPitchMinus4 = dstPitch - destBpp * sx;
					// slices are done individually
					for (size_t z = 0; z < depth; ++z)
					{
						// 4x4 blocks in x/y
						for (size_t y = 0; y < height; y += 4)
						{
							for (size_t x = 0; x < width; x += 4)
							{
								if (sourceFormat == IF_DXT2 || sourceFormat == IF_DXT3)
								{
									// explicit alpha
									stream->Read(&eAlpha, sizeof(DXTExplicitAlphaBlock));
									FlipEndian(eAlpha.alphaRow, sizeof(uint16_t), 4);
									UnpackDXTAlpha(eAlpha, tempColours);
								}
								else if (sourceFormat == IF_DXT4 || sourceFormat == IF_DXT5)
								{
									// interpolated alpha
									stream->Read(&iAlpha, sizeof(DXTInterpolatedAlphaBlock));
									FlipEndian(&(iAlpha.alpha_0), sizeof(uint16_t), 1);
									FlipEndian(&(iAlpha.alpha_1), sizeof(uint16_t), 1);
									UnpackDXTAlpha(iAlpha, tempColours);
								}
								// always read colour
								stream->Read(&col, sizeof(DXTColourBlock));
								FlipEndian(&(col.colour_0), sizeof(uint16_t), 1);
								FlipEndian(&(col.colour_1), sizeof(uint16_t), 1);
								UnpackDXTColour(sourceFormat, col, tempColours);

								// write 4x4 block to uncompressed version
								for (size_t by = 0; by < sy; ++by)
								{
									for (size_t bx = 0; bx < sx; ++bx)
									{
										auto PACK_COLOR = [](const ColourValue& value, ImageFormat format, void* dest)->bool
										{
											if (format == IF_R8G8B8)
											{
												uint32_t intValue = (uint32_t)(value.r * 0xFF) | ((uint32_t)(value.g * 0xFF) << 8) | ((uint32_t)(value.b * 0xFF) << 16);
												memcpy(dest, &intValue, sizeof(uint8_t) * 3);
												return true;
											}
											else if (format == IF_R8G8B8A8)
											{
												uint32_t intValue = (uint32_t)(value.r * 0xFF) | ((uint32_t)(value.g * 0xFF) << 8) | ((uint32_t)(value.b * 0xFF) << 16) | ((uint32_t)(value.a * 0xFF) << 24);
												memcpy(dest, &intValue, sizeof(uint8_t) * 4);
												return true;
											}
											return false;
										};
										ASSERT_RESULT(PACK_COLOR(tempColours[by * 4 + bx], result.eFormat, destPtr));
										destPtr = static_cast<void*>(static_cast<unsigned char*>(destPtr) + destBpp);
									}
									// advance to next row
									destPtr = static_cast<void*>(static_cast<unsigned char*>(destPtr) + destPitchMinus4);
								}
								// next block. Our dest pointer is 4 lines down
								// from where it started
								if (x + 4 >= width)
								{
									// Jump back to the start of the line
									destPtr = static_cast<void *>(static_cast<unsigned char*>(destPtr) - destPitchMinus4);
								}
								else
								{
									// Jump back up 4 rows and 4 pixels to the
									// right to be at the next block to the right
									destPtr = static_cast<void *>(static_cast<unsigned char*>(destPtr) - dstPitch * sy + destBpp * sx);
								}
							}
						}
					}
				}
				else
				{
					// load directly
					// DDS format lies! sizeOrPitch is not always set for DXT!!						
					size_t dxtSize = 0;
					ASSERT_RESULT(KImageHelper::GetByteSize(result.eFormat, width, height, depth, dxtSize));
					stream->Read(destPtr, dxtSize);
					destPtr = static_cast<void *>(static_cast<unsigned char*>(destPtr) + dxtSize);
				}
			}
			else // isCompress
			{
				// Get the byte size of the dest format element
				size_t elementByteSize = 0;
				ASSERT_RESULT(KImageHelper::GetElementByteSize(result.eFormat, elementByteSize));
				size_t dstPitch = width * elementByteSize;

				// Final data - trim incoming pitch
				size_t srcPitch;
				if (header.flags & DDSD_PITCH)
				{
					srcPitch = header.sizeOrPitch / std::max((size_t)1, mip * 2);
				}
				else
				{
					// assume same as final pitch
					srcPitch = dstPitch;
				}
				ASSERT_RESULT(dstPitch <= srcPitch);

				long srcAdvance = static_cast<long>(srcPitch) - static_cast<long>(dstPitch);
				for (size_t z = 0; z < depth; ++z)
				{
					for (size_t y = 0; y < height; ++y)
					{
						stream->Read(destPtr, dstPitch);
						if (srcAdvance > 0)
						{
							stream->Skip(srcAdvance);
						}
						destPtr = static_cast<void*>(static_cast<unsigned char*>(destPtr) + dstPitch);
					}
				}
			}

			KSubImageInfo subImageInfo;
			subImageInfo.uFaceIndex = face;
			subImageInfo.uMipmapIndex = mip;
			subImageInfo.uOffset = (size_t)srcData - (size_t)result.pData->GetData();
			subImageInfo.uSize = (size_t)destPtr - (size_t)srcData;
			subImageInfo.uWidth = width;
			subImageInfo.uHeight = height;

			subImageInfoList.push_back(subImageInfo);

			/// Next mip
			if (width != 1) width /= 2;
			if (height != 1) height /= 2;
			if (depth != 1) depth /= 2;
		}
	}

	return true;
}

bool KDDSCodec::Save(const KCodecResult& source, const char* pszFile)
{
	return false;
}

void KDDSCodec::FlipEndian(void *pData, size_t size, size_t count) const
{
#if NEED_FLIP_ENDIAN
	for (size_t index = 0; index < count; index++)
	{
		FlipEndian((void *)((size_t)pData + (index * size)), size);
	}
#endif
}

void KDDSCodec::FlipEndian(void *pData, size_t size) const
{
#if NEED_FLIP_ENDIAN
	char swapByte = EOF;
	for (unsigned int byteIndex = 0; byteIndex < size / 2; byteIndex++)
	{
		swapByte = *(char *)((size_t)pData + byteIndex);
		*(char *)((size_t)pData + byteIndex) = *(char *)((size_t)pData + size - byteIndex - 1);
		*(char *)((size_t)pData + size - byteIndex - 1) = swapByte;
	}
#endif
}