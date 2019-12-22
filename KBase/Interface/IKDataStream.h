#pragma once
#include "KBase/Publish/KConfig.h"
#include <memory>

struct IKDataStream;
typedef std::shared_ptr<IKDataStream> IKDataStreamPtr;

enum IOMode
{
	IM_READ = 0x01,
	IM_WRITE = 0x02,
	IM_READ_WRITE = 0x03,
	IM_INVALID = 0x04
};

enum IOType
{
	IT_MEMORY,
	IT_FILEHANDLE,
	IT_STREAM
};

enum IOLineMode
{
	// \n
	ILM_UNIX,
	// \r
	ILM_MAC,
	// \r\n
	ILM_WINDOWS,

	ILM_COUNT
};

struct IoLineDesc
{
	IOLineMode mode;
	const char* pszLine;
	unsigned char uLen;
};
const IoLineDesc LINE_DESCS[] =
{
	{ILM_UNIX, "\n", 1},
	{ILM_MAC, "\r", 1},
	{ILM_WINDOWS, "\r\n", 2},
	{ILM_COUNT, "", 0}
};
static_assert(ILM_COUNT + 1 == sizeof(LINE_DESCS) / sizeof(LINE_DESCS[0]), "ILM_COUNT NOT MATCH TO LINE_DESCS");

struct IKDataStream
{
	virtual ~IKDataStream() {}

	// 接口
	virtual bool Open(const char* pszFilePath, IOMode mode = IM_READ) = 0;
	virtual bool Open(const char* pDataBuffer, size_t uDataSize, IOMode mode = IM_READ) = 0;
	virtual bool Open(size_t uDataSize, IOMode mode = IM_WRITE) = 0;

	virtual bool Close() = 0;
	virtual bool IsEOF() = 0;
	virtual const char* GetFilePath() const = 0;
	virtual size_t GetSize() const = 0;
	virtual IOMode GetMode() const = 0;
	virtual IOType GetType() const = 0;
	virtual size_t Tell() const = 0;
	virtual size_t Seek(long nPos) = 0;
	virtual size_t Skip(size_t uSize) = 0;
	virtual bool Read(void* pBuffer, size_t uSize) = 0;
	virtual bool Write(const void* pBuffer, size_t uSize) = 0;
	virtual bool Reference(void** ppBuffer, size_t uSize) = 0;

	virtual bool IsReadable() const = 0;
	virtual bool IsWriteable() const = 0;
	virtual bool ReadLine(char* pszDestBuffer, size_t uBufferSize) = 0;
	virtual bool WriteLine(const char* pszLine, IOLineMode mode = ILM_UNIX) = 0;
};

EXPORT_DLL IKDataStreamPtr GetDataStream(IOType eType);