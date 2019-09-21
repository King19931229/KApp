#pragma once
#include "Publish/KConfig.h"

#include <memory>
#include <vector>
#include <assert.h>

enum ImageFormat
{
	IF_R8G8B8A8,
	IF_R8G8B8,

	IF_COUNT,
	IF_INVALID = IF_COUNT
};

class KImageData
{
protected:
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
		delete[] m_pData;
		m_pData = nullptr;
		m_uSize = 0;
	}
	size_t GetSize() const { return m_uSize; }
	unsigned char* GetData() { return m_pData; }
	const unsigned char* GetData() const { return m_pData; }
};
typedef std::shared_ptr<KImageData> KImageDataPtr;

struct KCodecResult
{
	bool bSuccess;
	ImageFormat eFormat;
	size_t uWidth;
	size_t uHeight;
	KImageDataPtr pData;
	KCodecResult()
	{
		bSuccess = false;
		eFormat = IF_INVALID;
		uWidth = 0;
		uHeight = 0;
	}
};

struct IKCodec;
typedef std::shared_ptr<IKCodec> IKCodecPtr;

struct IKCodec
{
	virtual KCodecResult Codec(const char* pszFile) = 0;
};

EXPORT_DLL bool InitCodecManager();
EXPORT_DLL bool UnInitCodecManager();
EXPORT_DLL IKCodecPtr GetCodec(const char* pszFile);