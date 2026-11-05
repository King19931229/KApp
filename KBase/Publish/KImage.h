#pragma once
#include <assert.h>
#include <memory>
#include <vector>
#include <limits>
#include <assert.h>

enum ImageFormat
{
	// 常规格式
	IF_R8G8B8A8,
	IF_R8G8B8,
	IF_R8G8,
	IF_R8,
	// 16bit浮点格式
	IF_R16_FLOAT,
	IF_R16G16_FLOAT,
	IF_R16G16B16_FLOAT,
	IF_R16G16B16A16_FLOAT,
	// 32bit浮点格式
	IF_R32_FLOAT,
	IF_R32G32_FLOAT,
	IF_R32G32B32_FLOAT,
	IF_R32G32B32A32_FLOAT,
	// 整型格式
	IF_R16_UINT,
	IF_R32_UINT,
	// ETC压缩格式
	IF_ETC1_RGB8,
	IF_ETC2_RGB8,
	IF_ETC2_RGB8A8,
	IF_ETC2_RGB8A1,
	// DDS压缩格式
	IF_DXT1,
	IF_DXT2,
	IF_DXT3,
	IF_DXT4,
	IF_DXT5,
	IF_BC4_UNORM,
	IF_BC4_SNORM,
	IF_BC5_UNORM,
	IF_BC5_SNORM,
	IF_BC6H_UF16,
	IF_BC6H_SF16,
	IF_BC7_UNORM,
	IF_BC7_UNORM_SRGB,
	// ASTC压缩格式
	IF_ASTC_4x4_UNORM,
	IF_ASTC_4x4_SRGB,
	IF_ASTC_5x4_UNORM,
	IF_ASTC_5x4_SRGB,
	IF_ASTC_5x5_UNORM,
	IF_ASTC_5x5_SRGB,
	IF_ASTC_6x5_UNORM,
	IF_ASTC_6x5_SRGB,
	IF_ASTC_6x6_UNORM,
	IF_ASTC_6x6_SRGB,
	IF_ASTC_8x5_UNORM,
	IF_ASTC_8x5_SRGB,
	IF_ASTC_8x6_UNORM,
	IF_ASTC_8x6_SRGB,
	IF_ASTC_8x8_UNORM,
	IF_ASTC_8x8_SRGB,
	IF_ASTC_10x5_UNORM,
	IF_ASTC_10x5_SRGB,
	IF_ASTC_10x6_UNORM,
	IF_ASTC_10x6_SRGB,
	IF_ASTC_10x8_UNORM,
	IF_ASTC_10x8_SRGB,
	IF_ASTC_10x10_UNORM,
	IF_ASTC_10x10_SRGB,
	IF_ASTC_12x10_UNORM,
	IF_ASTC_12x10_SRGB,
	IF_ASTC_12x12_UNORM,
	IF_ASTC_12x12_SRGB,

	// sRGB images and DDS block formats
	IF_DXT1_SRGB,
	IF_DXT3_SRGB,
	IF_DXT5_SRGB,
	IF_R8G8B8_SRGB,
	IF_R8G8B8A8_SRGB,

	IF_COUNT,
	IF_INVALID = IF_COUNT
};

struct KSubImageInfo
{
	size_t uWidth;
	size_t uHeight;
	size_t uOffset;
	size_t uSize;
	size_t uFaceIndex;
	size_t uSliceIndex;
	size_t uMipmapIndex;

	KSubImageInfo()
	{
		uWidth = 0;
		uHeight = 0;
		uOffset = 0;
		uSize = 0;
		uFaceIndex = 0;
		uSliceIndex = 0;
		uMipmapIndex = 0;
	}
};
typedef std::vector<KSubImageInfo> KSubImageInfoList;

class KImageData
{
protected:
	KSubImageInfoList m_SubImages;
	unsigned char* m_pData;
	size_t m_uSize;
public:
	KImageData(size_t uSize)
		: m_uSize(uSize)
	{
		assert(m_uSize > 0);
		m_pData = KNEW unsigned char[m_uSize];
		memset(m_pData, 0, m_uSize);
	}
	~KImageData()
	{
		assert(m_SubImages.size() > 0 && "SubImage information must exists");
		delete[] m_pData;
		m_pData = nullptr;
		m_uSize = 0;
	}

	size_t GetSize() const { return m_uSize; }

	KSubImageInfoList& GetSubImageInfo() { return m_SubImages; }
	const KSubImageInfoList& GetSubImageInfo() const { return m_SubImages; }

	unsigned char* GetData() { return m_pData; }
	const unsigned char* GetData() const { return m_pData; }
};
typedef std::shared_ptr<KImageData> KImageDataPtr;

namespace KImageHelper
{
	static bool IsValid(ImageFormat format)
	{
		return format >= 0 && format < IF_COUNT;
	}

	static bool GetIsCompress(ImageFormat format, bool& isCompress)
	{
		isCompress = false;
		if (!IsValid(format)) return false;
		switch (format)
		{
			case IF_ETC1_RGB8:
			case IF_ETC2_RGB8:
			case IF_ETC2_RGB8A8:
			case IF_ETC2_RGB8A1:

			case IF_DXT1:
			case IF_DXT1_SRGB:
			case IF_DXT2:
			case IF_DXT3:
			case IF_DXT3_SRGB:
			case IF_DXT4:
			case IF_DXT5:
			case IF_DXT5_SRGB:
			case IF_BC4_UNORM:
			case IF_BC4_SNORM:
			case IF_BC5_UNORM:
			case IF_BC5_SNORM:
			case IF_BC6H_UF16:
			case IF_BC6H_SF16:
			case IF_BC7_UNORM:
			case IF_BC7_UNORM_SRGB:

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

				isCompress = true;
				return true;
			default:
				isCompress = false;
				return true;
		}
	}

	static bool IsCompressed(ImageFormat format)
	{
		bool compressed = false;
		return GetIsCompress(format, compressed) && compressed;
	}

	static bool IsSRGBFormat(ImageFormat format)
	{
		switch (format)
		{
		case IF_BC7_UNORM_SRGB:
		case IF_ASTC_4x4_SRGB:
		case IF_ASTC_5x4_SRGB:
		case IF_ASTC_5x5_SRGB:
		case IF_ASTC_6x5_SRGB:
		case IF_ASTC_6x6_SRGB:
		case IF_ASTC_8x5_SRGB:
		case IF_ASTC_8x6_SRGB:
		case IF_ASTC_8x8_SRGB:
		case IF_ASTC_10x5_SRGB:
		case IF_ASTC_10x6_SRGB:
		case IF_ASTC_10x8_SRGB:
		case IF_ASTC_10x10_SRGB:
		case IF_ASTC_12x10_SRGB:
		case IF_ASTC_12x12_SRGB:
		case IF_DXT1_SRGB:
		case IF_DXT3_SRGB:
		case IF_DXT5_SRGB:
		case IF_R8G8B8_SRGB:
		case IF_R8G8B8A8_SRGB:
			return true;
		default:
			return false;
		}
	}

	static bool GetElementByteSize(ImageFormat format, size_t& size)
	{
		size = 0;
		switch (format)
		{
		case IF_R8G8B8A8:
		case IF_R16G16_FLOAT:
		case IF_R32_FLOAT:
		case IF_R32_UINT:
		case IF_R8G8B8A8_SRGB:
			size = 4;
			return true;
		case IF_R8G8B8:
		case IF_R8G8B8_SRGB:
			size = 3;
			return true;
		case IF_R8G8:
		case IF_R16_FLOAT:
		case IF_R16_UINT:
			size = 2;
			return true;
		case IF_R8:
			size = 1;
			return true;
		case IF_R16G16B16_FLOAT:
			size = 6;
			return true;
		case IF_R16G16B16A16_FLOAT:
		case IF_R32G32_FLOAT:
			size = 8;
			return true;
		case IF_R32G32B32_FLOAT:
			size = 12;
			return true;
		case IF_R32G32B32A32_FLOAT:
			size = 16;
			return true;
		default:
			return false;
		}
	}

	// https://stackoverflow.com/questions/17901939/what-is-the-block-size-of-etc2-compressed-texture
	static bool GetBlockByteSize(ImageFormat format, size_t& size)
	{
		switch (format)
		{
			case IF_ETC1_RGB8:
			case IF_ETC2_RGB8:
			case IF_ETC2_RGB8A1:
			case IF_DXT1:
			case IF_DXT1_SRGB:
			case IF_BC4_UNORM:
			case IF_BC4_SNORM:
				size = 8;
				return true;
			case IF_ETC2_RGB8A8:
			case IF_DXT2:
			case IF_DXT3:
			case IF_DXT3_SRGB:
			case IF_DXT4:
			case IF_DXT5:
			case IF_DXT5_SRGB:
			case IF_BC5_UNORM:
			case IF_BC5_SNORM:
			case IF_BC6H_UF16:
			case IF_BC6H_SF16:
			case IF_BC7_UNORM:
			case IF_BC7_UNORM_SRGB:
				size = 16;
				return true;
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
				size = 16;
				return true;

			default:
				assert(false && "unsupported format");
				return false;
		}
	}

	static bool GetBlockDimension(ImageFormat format, size_t& blockWidth, size_t& blockHeight)
	{
		switch (format)
		{
			case IF_ETC1_RGB8:
			case IF_ETC2_RGB8:
			case IF_ETC2_RGB8A8:
			case IF_ETC2_RGB8A1:
			case IF_DXT1:
			case IF_DXT1_SRGB:
			case IF_DXT2:
			case IF_DXT3:
			case IF_DXT3_SRGB:
			case IF_DXT4:
			case IF_DXT5:
			case IF_DXT5_SRGB:
			case IF_BC4_SNORM:
			case IF_BC4_UNORM:
			case IF_BC5_SNORM:
			case IF_BC5_UNORM:
			case IF_BC6H_SF16:
			case IF_BC6H_UF16:
			case IF_BC7_UNORM:
			case IF_BC7_UNORM_SRGB:
				blockWidth = 4;
				blockHeight = 4;
				return true;

			case IF_ASTC_4x4_UNORM:
			case IF_ASTC_4x4_SRGB:
				blockWidth = 4;
				blockHeight = 4;
				return true;

			case IF_ASTC_5x4_UNORM:
			case IF_ASTC_5x4_SRGB:
				blockWidth = 5;
				blockHeight = 4;
				return true;

			case IF_ASTC_5x5_UNORM:
			case IF_ASTC_5x5_SRGB:
				blockWidth = 5;
				blockHeight = 5;
				return true;

			case IF_ASTC_6x5_UNORM:
			case IF_ASTC_6x5_SRGB:
				blockWidth = 6;
				blockHeight = 5;
				return true;

			case IF_ASTC_6x6_UNORM:
			case IF_ASTC_6x6_SRGB:
				blockWidth = 6;
				blockHeight = 6;
				return true;

			case IF_ASTC_8x5_UNORM:
			case IF_ASTC_8x5_SRGB:
				blockWidth = 8;
				blockHeight = 5;
				return true;

			case IF_ASTC_8x6_UNORM:
			case IF_ASTC_8x6_SRGB:
				blockWidth = 8;
				blockHeight = 6;
				return true;

			case IF_ASTC_8x8_UNORM:
			case IF_ASTC_8x8_SRGB:
				blockWidth = 8;
				blockHeight = 8;
				return true;

			case IF_ASTC_10x5_UNORM:
			case IF_ASTC_10x5_SRGB:
				blockWidth = 10;
				blockHeight = 5;
				return true;

			case IF_ASTC_10x6_UNORM:
			case IF_ASTC_10x6_SRGB:
				blockWidth = 10;
				blockHeight = 6;
				return true;

			case IF_ASTC_10x8_UNORM:
			case IF_ASTC_10x8_SRGB:
				blockWidth = 10;
				blockHeight = 8;
				return true;

			case IF_ASTC_10x10_UNORM:
			case IF_ASTC_10x10_SRGB:
				blockWidth = 10;
				blockHeight = 10;
				return true;

			case IF_ASTC_12x10_UNORM:
			case IF_ASTC_12x10_SRGB:
				blockWidth = 12;
				blockHeight = 10;
				return true;

			case IF_ASTC_12x12_UNORM:
			case IF_ASTC_12x12_SRGB:
				blockWidth = 12;
				blockHeight = 12;
				return true;
		}

		blockWidth = 1;
		blockHeight = 1;
		return false;
	}

	namespace Detail
	{
		inline bool Multiply(size_t left, size_t right, size_t& result)
		{
			if (left && right > (std::numeric_limits<size_t>::max)() / left) return false;
			result = left * right;
			return true;
		}
	}

	static bool GetByteSize(ImageFormat format, size_t width, size_t height, size_t depth, size_t& size)
	{
		size = 0;
		if (!IsValid(format) || !width || !height || !depth) return false;
		size_t blockWidth = 1, blockHeight = 1, blockBytes = 0;
		if (IsCompressed(format))
		{
			if (!GetBlockDimension(format, blockWidth, blockHeight)
				|| !GetBlockByteSize(format, blockBytes)) return false;
		}
		else if (!GetElementByteSize(format, blockBytes)) return false;
		const size_t blocksX = width / blockWidth + (width % blockWidth != 0);
		const size_t blocksY = height / blockHeight + (height % blockHeight != 0);
		size_t bytes = 0;
		if (!Detail::Multiply(blocksX, blocksY, bytes)
			|| !Detail::Multiply(bytes, depth, bytes)
			|| !Detail::Multiply(bytes, blockBytes, bytes)) return false;
		size = bytes;
		return true;
	}

	static bool GetByteSize(ImageFormat format, size_t mipmaps, size_t faces,
		size_t width, size_t height, size_t depth, size_t& totalSize)
	{
		totalSize = 0;
		if (!mipmaps || !faces) return false;
		size_t total = 0;
		for (size_t mip = 0; mip < mipmaps; ++mip)
		{
			size_t bytes = 0;
			if (!GetByteSize(format, width, height, depth, bytes)
				|| !Detail::Multiply(bytes, faces, bytes)
				|| bytes > (std::numeric_limits<size_t>::max)() - total) return false;
			total += bytes;
			if (width > 1) width >>= 1;
			if (height > 1) height >>= 1;
			if (depth > 1) depth >>= 1;
		}
		totalSize = total;
		return true;
	}
}
