#include "KTextureBase.h"
#include "KBase/Interface/IKDataStream.h"
#include "Internal/KRenderGlobal.h"
#include <algorithm>
#include <cmath>

KTextureBase::KTextureBase()
	: m_Width(0),
	m_Height(0),
	m_Depth(0),
	m_Slice(0),
	m_Mipmaps(0),
	m_Format(PF_UNKNOWN),
	m_TextureType(TT_UNKNOWN),
	m_bGenerateMipmap(false),
	m_ResourceState(RS_UNLOADED)
{
}

KTextureBase::~KTextureBase()
{
	ASSERT_RESULT(!m_MemoryLoadTask.Get() || m_MemoryLoadTask->IsCompleted());
	ASSERT_RESULT(m_ResourceState == RS_UNLOADED);
}

bool KTextureBase::InitProperty(bool generateMipmap)
{
	if (!KPixelFormatHelper::IsValid(m_ImageData.eFormat)
		|| !m_ImageData.uWidth || !m_ImageData.uHeight || !m_ImageData.uDepth
		|| !m_ImageData.uSlice || !m_ImageData.uMipmap)
	{
		m_Format = PF_UNKNOWN;
		return false;
	}
	m_Format = m_ImageData.eFormat;
	m_ImageData.bCompressed = KPixelFormatHelper::IsCompressed(m_Format);
	m_Width = m_ImageData.uWidth;
	m_Height = m_ImageData.uHeight;
	m_Depth = m_ImageData.uDepth;
	m_Slice = m_ImageData.uSlice;

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
	m_Mipmaps = (uint32_t)(m_bGenerateMipmap ?
		(uint32_t)std::floor(std::log(std::max(std::max(m_Width, m_Height), m_Depth)) / std::log(2)) + 1 :
		m_ImageData.uMipmap);

	if (m_ImageData.bCubemap)
	{
		m_TextureType = TT_TEXTURE_CUBE_MAP;
	}
	else if (m_ImageData.bTextureArray)
	{
		m_TextureType = TT_TEXTURE_2D_ARRAY;
	}
	else if (m_Depth ==	1)
	{
		m_TextureType = TT_TEXTURE_2D;
	}
	else
	{
		m_TextureType = TT_TEXTURE_3D;
	}

	return true;
}

bool KTextureBase::CancelMemoryTask()
{
	if (m_MemoryLoadTask)
	{
		m_MemoryLoadTask->Abandon();
		m_MemoryLoadTask.Release();
	}
	return true;
}

bool KTextureBase::WaitMemoryTask()
{
	if (m_MemoryLoadTask)
	{
		m_MemoryLoadTask->WaitForCompletion();
		m_MemoryLoadTask.Release();
	}
	return true;
}

void KTextureBase::WaitForMemory()
{
	WaitMemoryTask();
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
		m_ImageData.pData = nullptr;
		m_ResourceState = RS_UNLOADED;
		return false;
	};

	if (async)
	{
		m_ResourceState = RS_MEMORY_PENDING;
		m_MemoryLoadTask = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork(loadImpl)), NamedThread::ANY_THREAD, {});
		return true;
	}
	else
	{
		return loadImpl();
	}
}

bool KTextureBase::InitMemoryFromData(const void* pRawData, const std::string& name, uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, bool cubeMap, bool bGenerateMipmap, bool async)
{
	ReleaseMemory();

	const uint32_t faceCount = cubeMap ? 6 : 1;
	size_t imageSize = 0;
	if (KPixelFormatHelper::IsCompressed(format)
		|| !KPixelFormatHelper::GetByteSize(format, 1, faceCount, width, height, depth, imageSize))
	{
		return false;
	}

	KImageDataPtr pImageData = KImageDataPtr(KNEW KImageData(imageSize));
	if (pRawData)
	{
		memcpy(pImageData->GetData(), pRawData, pImageData->GetSize());
	}

	for (uint32_t face = 0; face < faceCount; ++face)
	{
		KSubImageInfo subImageInfo;
		subImageInfo.uWidth = width;
		subImageInfo.uHeight = height;
		subImageInfo.uOffset = (pImageData->GetSize() / faceCount) * face;
		subImageInfo.uSize = pImageData->GetSize() / faceCount;
		subImageInfo.uFaceIndex = face;
		subImageInfo.uMipmapIndex = 0;
		pImageData->GetSubImageInfo().push_back(subImageInfo);
	}

	auto loadImpl = [=]()->bool
	{
		m_ResourceState = RS_MEMORY_LOADING;

		m_ImageData = KCodecResult();
		m_ImageData.eFormat = format;
		m_ImageData.uWidth = width;
		m_ImageData.uHeight = height;
		m_ImageData.uDepth = depth;
		m_ImageData.uMipmap = 1;
		m_ImageData.bCompressed = false;
		m_ImageData.bCubemap = cubeMap;
		m_ImageData.pData = pImageData;

		if (InitProperty(bGenerateMipmap))
		{
			m_Path = name;
			m_ResourceState = RS_MEMORY_LOADED;
			return true;
		}
		m_ImageData.pData = nullptr;
		m_ResourceState = RS_UNLOADED;
		return false;
	};

	if (async)
	{
		m_ResourceState = RS_MEMORY_PENDING;
		m_MemoryLoadTask = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork(loadImpl)), NamedThread::ANY_THREAD, {});
		return true;
	}
	else
	{
		return loadImpl();
	}
}

bool KTextureBase::InitMemoryFrom2DArray(const std::string& name, uint32_t width, uint32_t height, uint32_t slices, PixelFormat format, bool bGenerateMipmap, bool async)
{
	ReleaseMemory();

	auto loadImpl = [=]()->bool
	{
		size_t imageSize = 0;
		if (!KPixelFormatHelper::IsCompressed(format)
			&& KPixelFormatHelper::GetByteSize(format, 1, slices, width, height, 1, imageSize))
		{
			KImageDataPtr pImageData = KImageDataPtr(KNEW KImageData(imageSize));
			memset(pImageData->GetData(), 0, pImageData->GetSize());

			m_ImageData = KCodecResult();
			m_ImageData.eFormat = format;
			m_ImageData.uWidth = width;
			m_ImageData.uHeight = height;
			m_ImageData.uSlice = slices;
			m_ImageData.uDepth = 1;
			m_ImageData.uMipmap = 1;
			m_ImageData.bCompressed = false;
			m_ImageData.bCubemap = false;
			m_ImageData.bTextureArray = true;
			m_ImageData.pData = pImageData;

			for (uint32_t slice = 0; slice < slices; ++slice)
			{
				KSubImageInfo subImageInfo;
				subImageInfo.uWidth = width;
				subImageInfo.uHeight = height;
				subImageInfo.uOffset = (pImageData->GetSize() / slices) * slice;
				subImageInfo.uSize = pImageData->GetSize() / slices;
				subImageInfo.uSliceIndex = slice;
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

		m_ImageData.pData = nullptr;
		m_ResourceState = RS_UNLOADED;
		
		return false;
	};

	if (async)
	{
		m_ResourceState = RS_MEMORY_PENDING;
		m_MemoryLoadTask = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork(loadImpl)), NamedThread::ANY_THREAD, {});
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