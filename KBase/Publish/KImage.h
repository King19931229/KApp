#pragma once
#include <assert.h>
#include <memory>
#include <vector>
#include <assert.h>

enum ImageFormat
{
	// 常规格式
	IF_R8G8B8A8,
	IF_R8G8B8,
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
	size_t uMipmapIndex;

	KSubImageInfo()
	{
		uWidth = 0;
		uHeight = 0;
		uOffset = 0;
		uSize = 0;
		uFaceIndex = 0;
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
		m_pData = new unsigned char[m_uSize];
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
	static bool GetIsCompress(ImageFormat format, bool& isCompress)
	{
		switch (format)
		{
		case IF_ETC1_RGB8:
		case IF_ETC2_RGB8:
		case IF_ETC2_RGB8A8:
		case IF_ETC2_RGB8A1:

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

			isCompress = true;
			return true;
		default:
			isCompress = false;
			return true;
		}
	}

	static bool GetElementByteSize(ImageFormat format, size_t& size)
	{
		switch (format)
		{
		case IF_R8G8B8A8:
			size = 4;
			return true;
		case IF_R8G8B8:
			size = 3;
			return true;

		case IF_R16G16B16_FLOAT:
			size = 6;
			return true;
		case IF_R16G16B16A16_FLOAT:
			size = 8;
			return true;

		case IF_R32G32B32_FLOAT:
			size = 12;
			return true;
		case IF_R32G32B32A32_FLOAT:
			size = 16;
			return true;

		default:
			assert(false && "unsupported format");
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
			size = 8;
			return true;
		case IF_ETC2_RGB8A8:
		case IF_DXT2:
		case IF_DXT3:
		case IF_DXT4:
		case IF_DXT5:
			size = 16;
			return true;
		default:
			assert(false && "unsupported format");
			return false;
		}
	}

	static bool GetByteSize(ImageFormat format,
		size_t width, size_t height, size_t depth,
		size_t& size)
	{
		size = 0;
		bool isCompress = false;
		if(GetIsCompress(format, isCompress))
		{
			if(isCompress)
			{				
				switch (format)
				{
				case IF_ETC1_RGB8:
				case IF_ETC2_RGB8:
				case IF_ETC2_RGB8A8:
				case IF_ETC2_RGB8A1:
				case IF_DXT1:
				case IF_DXT2:
				case IF_DXT3:
				case IF_DXT4:
				case IF_DXT5:
				{
					size_t blockSize = 0;
					// Each 4 pixel form a block					
					if (GetBlockByteSize(format, blockSize))
					{
						size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize * depth;
						return true;
					}
					return false;
				}
				case IF_BC4_SNORM:
				case IF_BC4_UNORM:
				{
					size = (size_t)(ceilf(width / 4.0f) * ceilf(height / 4.0f) * 8.0f) * depth;
					return true;
				}
				case IF_BC5_SNORM:
				case IF_BC5_UNORM:
				case IF_BC6H_SF16:
				case IF_BC6H_UF16:
				case IF_BC7_UNORM:
				case IF_BC7_UNORM_SRGB:
				{
					size = (size_t)(ceilf(width / 4.0f) * ceilf(height / 4.0f) * 16.0f) * depth;
					return true;
				}
				default:
					assert(false && "unsuppored format");
					break;
				}
			}
			else
			{
				size_t elementSize = 0;
				if(GetElementByteSize(format, elementSize))
				{
					size = width * height * elementSize;
					return true;
				}
			}
		}
		return false;
	}

	static bool GetByteSize(ImageFormat format,
		size_t mipmaps, size_t faces,
		size_t width, size_t height, size_t depth,
		size_t& totalSize)
	{
		totalSize = 0;

		for(size_t m = 0; m < mipmaps; ++m)
		{
			size_t size = 0;
			if(GetByteSize(format, width, height, depth, size))
			{
				totalSize += size * faces;
			}
			else
			{
				return false;
			}
			if(width > 1) width >>= 1;
			if(height > 1) height >>= 1;
			if(depth > 1) depth >>= 1;
		}

		return true;
	}
}