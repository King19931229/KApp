#pragma once
#include "KConfig.h"
#include "KPixelFormat.h"
#include <assert.h>
#include <memory>
#include <vector>

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
