#include "KTextureBase.h"
#include "KBase/Interface/IKDataStream.h"
#include "Internal/KRenderGlobal.h"
#include <algorithm>
#include <cmath>

static bool ImageFormatToElementFormat(ImageFormat imageForamt, ElementFormat& elementFormat)
{
	switch (imageForamt)
	{
	case IF_R8G8B8A8:
		elementFormat = EF_R8G8B8A8_UNORM;
		return true;
	case IF_R8G8B8:
		elementFormat = EF_R8G8B8_UNORM;
		return true;
	case IF_R8G8:
		elementFormat = EF_R8G8_UNORM;
		return true;
	case IF_R8:
		elementFormat = EF_R8_UNORM;
		return true;

	case IF_R16_FLOAT:
		elementFormat = EF_R16_FLOAT;
		return true;
	case IF_R16G16_FLOAT:
		elementFormat = EF_R16G16_FLOAT;
		return true;
	case IF_R16G16B16_FLOAT:
		elementFormat = EF_R16G16B16_FLOAT;
		return true;
	case IF_R16G16B16A16_FLOAT:
		elementFormat = EF_R16G16B16A16_FLOAT;
		return true;

	case IF_R32_FLOAT:
		elementFormat = EF_R32_FLOAT;
		return true;
	case IF_R32G32_FLOAT:
		elementFormat = EF_R32G32_FLOAT;
		return true;
	case IF_R32G32B32_FLOAT:
		elementFormat = EF_R32G32B32_FLOAT;
		return true;
	case IF_R32G32B32A32_FLOAT:
		elementFormat = EF_R32G32B32A32_FLOAT;
		return true;

	case IF_R16_UINT:
		elementFormat = EF_R16_UINT;
		return true;

	case IF_R32_UINT:
		elementFormat = EF_R32_UINT;
		return true;

	case IF_ETC1_RGB8:
		elementFormat = EF_ETC1_R8G8B8_UNORM;
		return true;
	case IF_ETC2_RGB8:
		elementFormat = EF_ETC2_R8G8B8_UNORM;
		return true;		
	case IF_ETC2_RGB8A8:
		elementFormat = EF_ETC2_R8G8B8A8_UNORM;
		return true;
	case IF_ETC2_RGB8A1:
		elementFormat = EF_ETC2_R8G8B8A1_UNORM;
		return true;

	case IF_DXT1:
		// https://en.wikipedia.org/wiki/S3_Texture_Compression#DXT1
		elementFormat = EF_BC1_RGBA_UNORM;
		return true;
	case IF_DXT2:
		elementFormat = EF_BC2_UNORM;
		return true;
	case IF_DXT3:
		elementFormat = EF_BC2_UNORM;
		return true;
	case IF_DXT4:
		elementFormat = EF_BC3_UNORM;
		return true;
	case IF_DXT5:
		elementFormat = EF_BC3_UNORM;
		return true;
	case IF_BC4_UNORM:
		elementFormat = EF_BC4_UNORM;
		return true;
	case IF_BC4_SNORM:
		elementFormat = EF_BC4_SNORM;
		return true;
	case IF_BC5_UNORM:
		elementFormat = EF_BC5_SNORM;
		return true;
	case IF_BC5_SNORM:
		elementFormat = EF_BC5_SNORM;
		return true;
	case IF_BC6H_UF16:
		elementFormat = EF_BC6H_UFLOAT;
		return true;
	case IF_BC6H_SF16:
		elementFormat = EF_BC6H_SFLOAT;
		return true;
	case IF_BC7_UNORM:
		elementFormat = EF_BC7_UNORM;
		return true;
	case IF_BC7_UNORM_SRGB:
		elementFormat = EF_BC7_SRGB;
		return true;

	case IF_ASTC_4x4_UNORM:
		elementFormat = EF_ASTC_4x4_UNORM;
		return true;
	case IF_ASTC_4x4_SRGB:
		elementFormat = EF_ASTC_4x4_SRGB;
		return true;
	case IF_ASTC_5x4_UNORM:
		elementFormat = EF_ASTC_5x4_UNORM;
		return true;
	case IF_ASTC_5x4_SRGB:
		elementFormat = EF_ASTC_5x4_SRGB;
		return true;
	case IF_ASTC_5x5_UNORM:
		elementFormat = EF_ASTC_5x5_UNORM;
		return true;
	case IF_ASTC_5x5_SRGB:
		elementFormat = EF_ASTC_5x5_SRGB;
		return true;
	case IF_ASTC_6x5_UNORM:
		elementFormat = EF_ASTC_6x5_UNORM;
		return true;
	case IF_ASTC_6x5_SRGB:
		elementFormat = EF_ASTC_6x5_SRGB;
		return true;
	case IF_ASTC_6x6_UNORM:
		elementFormat = EF_ASTC_6x6_UNORM;
		return true;
	case IF_ASTC_6x6_SRGB:
		elementFormat = EF_ASTC_6x6_SRGB;
		return true;
	case IF_ASTC_8x5_UNORM:
		elementFormat = EF_ASTC_8x5_UNORM;
		return true;
	case IF_ASTC_8x5_SRGB:
		elementFormat = EF_ASTC_8x5_SRGB;
		return true;
	case IF_ASTC_8x6_UNORM:
		elementFormat = EF_ASTC_8x6_UNORM;
		return true;
	case IF_ASTC_8x6_SRGB:
		elementFormat = EF_ASTC_8x6_SRGB;
		return true;
	case IF_ASTC_8x8_UNORM:
		elementFormat = EF_ASTC_8x8_UNORM;
		return true;
	case IF_ASTC_8x8_SRGB:
		elementFormat = EF_ASTC_8x8_SRGB;
		return true;
	case IF_ASTC_10x5_UNORM:
		elementFormat = EF_ASTC_10x5_UNORM;
		return true;
	case IF_ASTC_10x5_SRGB:
		elementFormat = EF_ASTC_10x5_SRGB;
		return true;
	case IF_ASTC_10x6_UNORM:
		elementFormat = EF_ASTC_10x6_UNORM;
		return true;
	case IF_ASTC_10x6_SRGB:
		elementFormat = EF_ASTC_10x6_SRGB;
		return true;
	case IF_ASTC_10x8_UNORM:
		elementFormat = EF_ASTC_10x8_UNORM;
		return true;
	case IF_ASTC_10x8_SRGB:
		elementFormat = EF_ASTC_10x8_SRGB;
		return true;
	case IF_ASTC_10x10_UNORM:
		elementFormat = EF_ASTC_10x10_UNORM;
		return true;
	case IF_ASTC_10x10_SRGB:
		elementFormat = EF_ASTC_10x10_SRGB;
		return true;
	case IF_ASTC_12x10_UNORM:
		elementFormat = EF_ASTC_12x10_UNORM;
		return true;
	case IF_ASTC_12x10_SRGB:
		elementFormat = EF_ASTC_12x10_SRGB;
		return true;
	case IF_ASTC_12x12_UNORM:
		elementFormat = EF_ASTC_12x12_UNORM;
		return true;
	case IF_ASTC_12x12_SRGB:
		elementFormat = EF_ASTC_12x12_SRGB;
		return true;

	case IF_COUNT:
	default:
		assert(false && "unsupport format");
		elementFormat = EF_UNKNOWN;
		return false;
	}
}

// TODO 补充
static bool ImageFormatToSize(ImageFormat format, size_t& size)
{
	switch (format)
	{
	case IF_R8G8B8A8:
		size = 4;
		return true;
	case IF_R8G8B8:
		size = 3;
		return true;
	case IF_R8G8:
		size = 2;
		return true;
	case IF_R8:
		size = 1;
		return true;
	case IF_R16G16B16A16_FLOAT:
		size = 8;
		return true;
	case IF_R16G16B16_FLOAT:
		size = 6;
		return true;
	case IF_R16G16_FLOAT:
		size = 4;
		return true;
	case IF_R16_FLOAT:
		size = 2;
		return true;
	case IF_R32G32B32A32_FLOAT:
		size = 16;
		return true;
	case IF_R32G32B32_FLOAT:
		size = 12;
		return true;
	case IF_R32G32_FLOAT:
		size = 8;
		return true;
	case IF_R32_FLOAT:
		size = 4;
		return true;
	case IF_COUNT:
		break;
	default:
		break;
	}
	assert(false && "unsupport format");
	size = 0;
	return false;
}

KTextureBase::KTextureBase()
	: m_Width(0),
	m_Height(0),
	m_Depth(0),
	m_Mipmaps(0),
	m_Format(EF_UNKNOWN),
	m_TextureType(TT_UNKNOWN),
	m_bGenerateMipmap(false),
	m_ResourceState(RS_UNLOADED),
	m_MemoryLoadTask(nullptr)
{
}

KTextureBase::~KTextureBase()
{
	ASSERT_RESULT(m_MemoryLoadTask == nullptr);
	ASSERT_RESULT(m_ResourceState == RS_UNLOADED);
}

bool KTextureBase::InitProperty(bool generateMipmap)
{
	m_Width = m_ImageData.uWidth;
	m_Height = m_ImageData.uHeight;
	m_Depth = m_ImageData.uDepth;

	// 已经存在mipmap数据或者格式为压缩格式就不硬生成mipmap
	if (m_ImageData.uMipmap > 1 || m_ImageData.bCompressed)
	{
		m_bGenerateMipmap = false;
	}
	else
	{
		m_bGenerateMipmap = generateMipmap;
	}
	// 如果硬生成mipmap mipmap层数与尺寸相关 否则从mipmap数据中获取
	m_Mipmaps = (unsigned short)(m_bGenerateMipmap ?
		(unsigned short)std::floor(std::log(std::max(std::max(m_Width, m_Height), m_Depth)) / std::log(2)) + 1 :
		m_ImageData.uMipmap);

	if (m_ImageData.bCubemap)
	{
		m_TextureType = TT_TEXTURE_CUBE_MAP;
	}
	else if (m_Depth ==	1)
	{
		m_TextureType = TT_TEXTURE_2D;
	}
	else
	{
		m_TextureType = TT_TEXTURE_3D;
	}

	if(ImageFormatToElementFormat(m_ImageData.eFormat, m_Format))
	{
		return true;
	}
	return false;
}

bool KTextureBase::CancelMemoryTask()
{
	std::unique_lock<decltype(m_MemoryLoadTaskLock)> guard(m_MemoryLoadTaskLock);
	if (m_MemoryLoadTask)
	{
		m_MemoryLoadTask->Cancel();
		m_MemoryLoadTask = nullptr;
	}
	return true;
}

bool KTextureBase::WaitMemoryTask()
{
	std::unique_lock<decltype(m_MemoryLoadTaskLock)> guard(m_MemoryLoadTaskLock);
	if (m_MemoryLoadTask)
	{
		m_MemoryLoadTask->Wait();
		m_MemoryLoadTask = nullptr;
	}
	return true;
}

bool KTextureBase::ReleaseMemory()
{
	WaitMemoryTask();
	m_ImageData.pData = nullptr;
	m_ResourceState = RS_UNLOADED;
	return true;
}

bool KTextureBase::InitMemoryFromFile(const std::string& filePath, bool bGenerateMipmap, bool async)
{
	ReleaseMemory();
	auto loadImpl = [=]()->bool
	{
		m_ResourceState = RS_MEMORY_LOADING;

		IKCodecPtr pCodec = KCodec::GetCodec(filePath.c_str());
		if (pCodec && pCodec->Codec(filePath.c_str(), true, m_ImageData))
		{
			if (InitProperty(bGenerateMipmap))
			{
				m_Path = filePath;
				m_ResourceState = RS_MEMORY_LOADED;
				return true;
			}
		}

		m_ResourceState = RS_UNLOADED;
		m_ImageData.pData = nullptr;
		return false;
	};

	if (async)
	{
		std::unique_lock<decltype(m_MemoryLoadTaskLock)> guard(m_MemoryLoadTaskLock);
		m_MemoryLoadTask = KRenderGlobal::TaskExecutor.Submit(KTaskUnitPtr(KNEW KSampleAsyncTaskUnit(loadImpl)));
		return true;
	}
	else
	{
		return loadImpl();
	}
}

bool KTextureBase::InitMemoryFromData(const void* pRawData, const std::string& name, size_t width, size_t height, size_t depth, ImageFormat format, bool cubeMap, bool bGenerateMipmap, bool async)
{
	ReleaseMemory();
	auto loadImpl = [=]()->bool
	{
		m_ResourceState = RS_MEMORY_LOADING;

		size_t formatSize = 0;
		if (ImageFormatToSize(format, formatSize))
		{
			size_t faceCount = cubeMap ? 6 : 1;
			KImageDataPtr pImageData = KImageDataPtr(KNEW KImageData(width * height * depth * faceCount * formatSize));

			if (pRawData)
			{
				memcpy(pImageData->GetData(), pRawData, pImageData->GetSize());
			}
			else
			{
				memset(pImageData->GetData(), 0, pImageData->GetSize());
			}

			m_ImageData.eFormat = format;
			m_ImageData.uWidth = width;
			m_ImageData.uHeight = height;
			m_ImageData.uDepth = depth;
			m_ImageData.uMipmap = 1;
			m_ImageData.bCompressed = false;
			m_ImageData.bCubemap = cubeMap;
			m_ImageData.pData = pImageData;

			for (size_t face = 0; face < faceCount; ++face)
			{
				KSubImageInfo subImageInfo;
				subImageInfo.uWidth = width;
				subImageInfo.uHeight = height;
				subImageInfo.uOffset = pImageData->GetSize() * face / faceCount;
				subImageInfo.uSize = pImageData->GetSize();
				subImageInfo.uFaceIndex = face;
				subImageInfo.uMipmapIndex = 0;
				m_ImageData.pData->GetSubImageInfo().push_back(subImageInfo);
			}

			if (InitProperty(bGenerateMipmap))
			{
				m_Path = name;
				m_ResourceState = RS_MEMORY_LOADED;
				return true;
			}
		}

		m_ResourceState = RS_UNLOADED;
		m_ImageData.pData = nullptr;
		return false;
	};

	if (async)
	{
		m_ResourceState = RS_PENDING;
		std::unique_lock<decltype(m_MemoryLoadTaskLock)> guard(m_MemoryLoadTaskLock);
		m_MemoryLoadTask = KRenderGlobal::TaskExecutor.Submit(KTaskUnitPtr(KNEW KSampleAsyncTaskUnit(loadImpl)));
		return true;
	}
	else
	{
		return loadImpl();
	}
}

bool KTextureBase::UnInit()
{
	ReleaseMemory();
	return true;
}