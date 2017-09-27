#pragma once
#include "Interface/IKDataStream.h"

#include <fstream>

class KDataStreamBase : public IKDataStream
{
public:
	virtual bool IsReadable() const { return (GetMode() & IM_READ) > 0; }
	virtual bool IsWriteable() const { return (GetMode() & IM_WRITE) > 0; }

	virtual bool ReadLine(char* pszDestBuffer, size_t uBufferSize);
	virtual bool WriteLine(const char* pszLine);
};

class KMemoryDataStream : public KDataStreamBase
{
protected:
	char* m_pDataBuffer;
	char* m_pCurPos;
	size_t m_uBufferSize;
	IOMode m_eMode;

	void ReleaseData();
public:
	KMemoryDataStream();
	~KMemoryDataStream();

	virtual bool Open(const char* pszFilePath, IOMode mode);
	virtual bool Open(const char* pDataBuffer, size_t uDataSize, IOMode mode);
	virtual bool Open(size_t uDataSize, IOMode mode);
	//virtual bool Clone(IKDataStreamPtr pSrc) = 0;
	virtual bool Close();
	virtual bool IsEOF();
	virtual const char* GetFilePath() const;
	virtual size_t GetSize() const;
	virtual IOMode GetMode() const;
	virtual IOType GetType() const;
	virtual size_t Tell() const;
	virtual size_t Seek(long nPos);
	virtual bool Read(char* pszBuffer, size_t uSize);
	virtual bool Write(const char* pszBuffer, size_t uSize);
};

class KFileHandleDataStream : public KDataStreamBase
{
protected:
	FILE* m_pFile;
	IOMode m_eMode;
	size_t m_uSize;
	std::string m_FilePath;

	void ReleaseHandle();
public:
	KFileHandleDataStream();
	~KFileHandleDataStream();

	virtual bool Open(const char* pszFilePath, IOMode mode);
	virtual bool Open(const char* pDataBuffer, size_t uDataSize, IOMode mode);
	virtual bool Open(size_t uDataSize, IOMode mode);
	//virtual bool Clone(IKDataStreamPtr pSrc) = 0;
	virtual bool Close();
	virtual bool IsEOF();
	virtual const char* GetFilePath() const;
	virtual size_t GetSize() const;
	virtual IOMode GetMode() const;
	virtual IOType GetType() const;
	virtual size_t Tell() const;
	virtual size_t Seek(long nPos);
	virtual bool Read(char* pszBuffer, size_t uSize);
	virtual bool Write(const char* pszBuffer, size_t uSize);
};

class KFileDataStream : public KDataStreamBase
{
protected:
	std::fstream m_FileStream;
	IOMode m_eMode;
	size_t m_uSize;
	std::string m_FilePath;

	void ReleaseStream();
public:
	KFileDataStream();
	~KFileDataStream();

	virtual bool Open(const char* pszFilePath, IOMode mode);
	virtual bool Open(const char* pDataBuffer, size_t uDataSize, IOMode mode);
	virtual bool Open(size_t uDataSize, IOMode mode);
	//virtual bool Clone(IKDataStreamPtr pSrc) = 0;
	virtual bool Close();
	virtual bool IsEOF();
	virtual const char* GetFilePath() const;
	virtual size_t GetSize() const;
	virtual IOMode GetMode() const;
	virtual IOType GetType() const;
	virtual size_t Tell() const;
	virtual size_t Seek(long nPos);
	virtual bool Read(char* pszBuffer, size_t uSize);
	virtual bool Write(const char* pszBuffer, size_t uSize);
};