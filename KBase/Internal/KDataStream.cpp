#include "Internal/KDataStream.h"

#include <assert.h>

IKDataStreamPtr GetDataStream(IOType eType)
{
	IKDataStreamPtr pRet;
	switch (eType)
	{
	case IT_MEMORY:
		pRet = IKDataStreamPtr(new KMemoryDataStream());
		break;
	case IT_FILEHANDLE:
		pRet = IKDataStreamPtr(new KFileHandleDataStream());
		break;
	case IT_STREAM:
		pRet = IKDataStreamPtr(new KFileDataStream());
		break;
	default:
		break;
	}
	return pRet;
}

// KDataStreamBase
KDataStreamBase::~KDataStreamBase()
{
}

bool KDataStreamBase::_ReadLine(const char* pszLine, size_t uLen, IOLineMode* pMode, const char** ppRetEndPos)
{
	if(pszLine && uLen > 0)
	{
		const char* pFindPos = nullptr;
		IOLineMode eCurLineMode = ILM_COUNT;

		for(pFindPos = pszLine; pFindPos;)
		{
			if(pFindPos - pszLine == uLen)
			{
				pFindPos = nullptr;
				break;
			}
			// windows
			if((size_t)(pFindPos - pszLine + 1) < uLen)
			{
				if(*pFindPos == '\r' && *(pFindPos + 1) == '\n')
				{
					++pFindPos;
					eCurLineMode = ILM_WINDOWS;
					break;
				}
			}
			// fallthough unix
			if(*pFindPos == '\n')
			{
				eCurLineMode = ILM_UNIX;
				break;
			}
			// fallthough mac
			if(*pFindPos == '\r')
			{
				eCurLineMode = ILM_MAC;
				break;
			}
			++pFindPos;
		}

		if(pFindPos)
		{
			if(ppRetEndPos)
				*ppRetEndPos = pFindPos;
			if(pMode)
				*pMode = eCurLineMode;
			return true;
		}
	}
	return false;
}

bool KDataStreamBase::ReadLine(char* pszDestBuffer, size_t uBufferSize)
{
	char szBuffer[256] = {0};
	bool bFind = false;
	if(pszDestBuffer && uBufferSize > 0)
	{
		size_t uLastReadPos = Tell();
		size_t uBufferRestSize = uBufferSize - 1;
		size_t uSize = GetSize();
		size_t uFileRestSize = uSize - uLastReadPos;
		size_t uCurReadSize = 0;
		size_t uTotalReadSize = 0;
		IOLineMode eCurLineMode = ILM_COUNT;
		char* pFindPos = nullptr;
		const char* pLineEndPos = nullptr;
		while(!IsEOF())
		{
			uCurReadSize = std::min(sizeof(szBuffer), uBufferRestSize);
			uCurReadSize = std::min(uCurReadSize, uFileRestSize);
			if(uCurReadSize == 0 || !Read(pszDestBuffer, uCurReadSize))
				break;
			eCurLineMode = ILM_COUNT;
			// 找换行
			if(_ReadLine(pszDestBuffer, uCurReadSize, &eCurLineMode, &pLineEndPos))
			{
				pFindPos = pszDestBuffer + (pLineEndPos - pszDestBuffer);
				switch (eCurLineMode)
				{
				case ILM_WINDOWS:
					*(pFindPos - 1) = '\0';
					// fallthough
				case ILM_UNIX:
				case ILM_MAC:
					*pFindPos = '\0';
					bFind = true;
					break;
				default:
					assert(false && "never reach");
					break;
				}
				uCurReadSize = pFindPos - pszDestBuffer + 1;
			}

			// 已经读完
			if(uFileRestSize == uCurReadSize)
			{
				pszDestBuffer[uFileRestSize] = '\0';
				bFind = true;
			}

			uFileRestSize -= uCurReadSize;
			uBufferRestSize -= uCurReadSize;
			uTotalReadSize += uCurReadSize;
			pszDestBuffer += uCurReadSize;

			if(bFind)
			{
				Seek((long)(uLastReadPos + uTotalReadSize));
				// 成功出口
				return true;
			}
		}

		if(uTotalReadSize)
			Seek((long)uLastReadPos);
	}
	return false;
}

bool KDataStreamBase::WriteLine(const char* pszLine, IOLineMode mode)
{
	if(pszLine)
	{
		size_t uCurWritePos = Tell();
		size_t uLen = strlen(pszLine);
		if(uLen)
		{
			const char* pLineEndPos = nullptr;
			IOLineMode eCurLineMode = ILM_COUNT;
			if(_ReadLine(pszLine, uLen, &eCurLineMode, &pLineEndPos))
			{
				assert(pLineEndPos);(pLineEndPos);
				uLen -= LINE_DESCS[eCurLineMode].uLen;
				assert(uLen > 0);
			}
			if(Write(pszLine, uLen) && Write(LINE_DESCS[mode].pszLine, LINE_DESCS[mode].uLen))
				return true;
			Seek((long)uCurWritePos);
		}
	}
	return false;
}

// KMemoryDataStream
KMemoryDataStream::KMemoryDataStream()
	: m_pDataBuffer(nullptr),
	m_pCurPos(nullptr),
	m_uBufferSize(0),
	m_eMode(IM_INVALID)
{
}

KMemoryDataStream::~KMemoryDataStream()
{
	ReleaseData();
}

void KMemoryDataStream::ReleaseData()
{
	if(m_pDataBuffer)
	{
		delete[] m_pDataBuffer;
		m_pDataBuffer = nullptr;
	}
	m_pCurPos = nullptr;
	m_uBufferSize = 0;
}

bool KMemoryDataStream::Open(const char* pszFilePath, IOMode mode)
{
	ReleaseData();
	if(pszFilePath && mode == IM_READ)
	{
		IKDataStreamPtr pData = GetDataStream(IT_FILEHANDLE);
		if(pData->Open(pszFilePath, IM_READ))
		{
			size_t uDataSize = pData->GetSize();
			if(uDataSize > 0)
			{
				m_pDataBuffer = new char[uDataSize];
				memset(m_pDataBuffer, 0, sizeof(m_pDataBuffer[0]) * uDataSize);
				m_pCurPos = m_pDataBuffer;
				m_uBufferSize = uDataSize;
				m_eMode = mode;
				if(pData->Read(m_pDataBuffer, uDataSize))
					return true;
				ReleaseData();
			}
			else
			{
				return true;
			}
		}
	}
	return false;
}

bool KMemoryDataStream::Open(const char* pDataBuffer, size_t uDataSize, IOMode mode)
{
	ReleaseData();
	if(pDataBuffer && uDataSize > 0)
	{
		m_pDataBuffer = new char[uDataSize];
		memcpy(m_pDataBuffer, pDataBuffer, sizeof(pDataBuffer[0]) * uDataSize);
		m_pCurPos = m_pDataBuffer;
		m_uBufferSize = uDataSize;
		m_eMode = mode;
		return true;
	}
	return false;
}

bool KMemoryDataStream::Open(size_t uDataSize, IOMode mode)
{
	ReleaseData();
	if(uDataSize > 0)
	{
		m_pDataBuffer = new char[uDataSize];
		memset(m_pDataBuffer, 0, sizeof(m_pDataBuffer[0]) * uDataSize);
		m_pCurPos = m_pDataBuffer;
		m_uBufferSize = uDataSize;
		m_eMode = mode;
		return true;
	}
	return false;
}

bool KMemoryDataStream::IsEOF()
{
	return m_pCurPos >= (m_pDataBuffer + m_uBufferSize);
}

bool KMemoryDataStream::Close()
{
	ReleaseData();
	return true;
}

const char* KMemoryDataStream::GetFilePath() const
{
	return nullptr;
}

size_t KMemoryDataStream::GetSize() const
{
	return m_uBufferSize;
}

IOMode KMemoryDataStream::GetMode() const
{
	return m_eMode;
}

IOType KMemoryDataStream::GetType() const
{
	return IT_MEMORY;
}

size_t KMemoryDataStream::Tell() const
{
	return (size_t)(m_pCurPos - m_pDataBuffer);
}

size_t KMemoryDataStream::Seek(long nPos)
{
	if(nPos < 0 || (size_t)nPos > m_uBufferSize)
		return false;
	m_pCurPos = m_pDataBuffer + nPos;
	return (size_t)nPos;
}

bool KMemoryDataStream::Read(char* pszBuffer, size_t uSize)
{
	if(!IsReadable())
		return false;
	size_t uLeftSize = m_uBufferSize - (size_t)(m_pCurPos - m_pDataBuffer);
	if(uSize > uLeftSize)
		return false;
	memcpy(pszBuffer, m_pCurPos, sizeof(pszBuffer[0]) * uSize);
	m_pCurPos += uSize;
	return true;
}

bool KMemoryDataStream::Write(const char* pszBuffer, size_t uSize)
{
	if(!IsWriteable())
		return false;
	size_t uLeftSize = m_uBufferSize - (size_t)(m_pCurPos - m_pDataBuffer);
	if(uSize > uLeftSize)
		return false;
	memcpy(m_pCurPos, pszBuffer, sizeof(pszBuffer[0]) * uSize);
	m_pCurPos += uSize;
	return true;
}

bool KMemoryDataStream::Reference(char** ppszBuffer, size_t uSize)
{
	if(ppszBuffer)
	{
		size_t uLeftSize = m_uBufferSize - (size_t)(m_pCurPos - m_pDataBuffer);
		if(uSize <= uLeftSize)
		{
			*ppszBuffer = m_pCurPos;
			m_pCurPos += uSize;
			return true;
		}
	}
	return false;
}

// KFileHandleDataStream
KFileHandleDataStream::KFileHandleDataStream()
	: m_pFile(nullptr),
	m_eMode(IM_INVALID),
	m_uSize(0)
{
}

KFileHandleDataStream::~KFileHandleDataStream()
{
	ReleaseHandle();
}

void KFileHandleDataStream::ReleaseHandle()
{
	if(m_pFile)
		fclose(m_pFile);
	m_pFile = nullptr;
	m_uSize = 0;
	m_FilePath.clear();
}

bool KFileHandleDataStream::Open(const char* pszFilePath, IOMode mode)
{
	ReleaseHandle();
	if(pszFilePath)
	{
		const char* ppszMode[] = {"", "rb", "wb", "wb+"};
		unsigned char uIndex = 0;

		switch (mode)
		{
		case IM_READ:
			uIndex = 1;
			break;
		case IM_WRITE:
			uIndex = 2;
			break;
		case IM_READ_WRITE:
			uIndex = 3;
			break;
		case IM_INVALID:
			break;
		default:
			break;
		}
		if(uIndex > 0)
#ifndef _WIN32
			m_pFile = fopen(pszFilePath, ppszMode[uIndex]);
#else
			fopen_s(&m_pFile, pszFilePath, ppszMode[uIndex]);
#endif
	}

	if(m_pFile)
	{
		fseek(m_pFile, 0, SEEK_END);
		m_uSize = (size_t)ftell(m_pFile);
		fseek(m_pFile, 0, SEEK_SET);
		m_FilePath = pszFilePath;
		m_eMode = mode;
	}

	return m_pFile != nullptr;
}

bool KFileHandleDataStream::Open(const char* pDataBuffer, size_t uDataSize, IOMode mode)
{
	return false;
}

bool KFileHandleDataStream::Open(size_t uDataSize, IOMode mode)
{
	return false;
}

bool KFileHandleDataStream::Close()
{
	ReleaseHandle();
	return true;
}

bool KFileHandleDataStream::IsEOF()
{
	if(m_pFile)
		return feof(m_pFile) != 0;
	return true;
}

const char* KFileHandleDataStream::GetFilePath() const
{
	return m_FilePath.c_str();
}

size_t KFileHandleDataStream::GetSize() const
{
	return m_uSize;
}

IOMode KFileHandleDataStream::GetMode() const
{
	return m_eMode;
}

IOType KFileHandleDataStream::GetType() const
{
	return IT_FILEHANDLE;
}

size_t KFileHandleDataStream::Tell() const
{
	if(m_pFile)
		return (size_t)ftell(m_pFile);
	else
		return 0;
}

size_t KFileHandleDataStream::Seek(long nPos)
{
	if(m_pFile)
		return (size_t)fseek(m_pFile, nPos, SEEK_SET);
	else
		return 0;
}

bool KFileHandleDataStream::Read(char* pszBuffer, size_t uSize)
{
	if(!IsReadable())
		return false;
	if(m_pFile)
		return fread(pszBuffer, 1, uSize, m_pFile) == uSize;
	else
		return false;
}

bool KFileHandleDataStream::Write(const char* pszBuffer, size_t uSize)
{
	if(!IsWriteable())
		return false;
	if(m_pFile)
		return fwrite(pszBuffer, 1, uSize, m_pFile) == uSize;
	else
		return false;
}

bool KFileHandleDataStream::Reference(char** ppszBuffer, size_t uSize)
{
	if(ppszBuffer)
		*ppszBuffer = nullptr;
	return false;
}

// KFileDataStream
KFileDataStream::KFileDataStream()
	: m_eMode(IM_INVALID),
	m_uSize(0)
{
}

KFileDataStream::~KFileDataStream()
{
	ReleaseStream();
}

void KFileDataStream::ReleaseStream()
{
	m_FileStream.close();
	m_eMode = IM_INVALID;
	m_uSize = 0;
	m_FilePath.clear();
}

bool KFileDataStream::Open(const char* pszFilePath, IOMode mode)
{
	ReleaseStream();
	if(pszFilePath)
	{
		std::ios_base::open_mode uMode = std::ios_base::binary;

		switch (mode)
		{
		case IM_READ:
			uMode |= std::ios_base::in;
			m_FileStream.open(pszFilePath, uMode);
			break;
		case IM_WRITE:
			uMode |= std::ios_base::out;
			m_FileStream.open(pszFilePath, uMode);
			break;
		case IM_READ_WRITE:
			uMode |= std::ios_base::in | std::ios_base::out;
			m_FileStream.open(pszFilePath, uMode);
			break;
		case IM_INVALID:
			break;
		default:
			break;
		}

		if(m_FileStream.is_open())
		{
			m_eMode = mode;
			m_FileStream.seekg(0, std::ios_base::end);
			m_uSize = (size_t)m_FileStream.tellg();
			m_FileStream.seekg(0, std::ios_base::beg);
			m_FilePath = pszFilePath;
			return true;
		}
	}
	return false;
}

bool KFileDataStream::Open(const char* pDataBuffer, size_t uDataSize, IOMode mode)
{
	return false;
}

bool KFileDataStream::Open(size_t uDataSize, IOMode mode)
{
	return false;
}

bool KFileDataStream::Close()
{
	if(m_FileStream.is_open())
		m_FileStream.close();
	return true;
}

bool KFileDataStream::IsEOF()
{
	if(m_FileStream.is_open())
		return m_FileStream.eof();
	return true;
}

const char* KFileDataStream::GetFilePath() const
{
	return m_FilePath.c_str();
}

size_t KFileDataStream::GetSize() const
{
	return m_uSize;
}

IOMode KFileDataStream::GetMode() const
{
	return m_eMode;
}

IOType KFileDataStream::GetType() const
{
	return IT_STREAM;
}

size_t KFileDataStream::Tell() const
{
	if(m_FileStream.is_open())
		return (size_t)const_cast<std::fstream*>(&m_FileStream)->tellg();
	else
		return 0;
}

size_t KFileDataStream::Seek(long nPos)
{
	if(m_FileStream.is_open())
	{
		m_FileStream.seekg((std::ios::pos_type)nPos, std::ios_base::beg);
		return (size_t)m_FileStream.tellg();
	}
	else
		return 0;
}

bool KFileDataStream::Read(char* pszBuffer, size_t uSize)
{
	if(!IsReadable())
		return false;
	if(m_FileStream.is_open())
	{
		m_FileStream.read(pszBuffer, (std::streamsize)uSize);
		return m_FileStream.gcount() == uSize;
	}
	else
		return false;
}

bool KFileDataStream::Write(const char* pszBuffer, size_t uSize)
{
	if(!IsWriteable())
		return false;
	if(m_FileStream.is_open())
	{
		m_FileStream.write(pszBuffer, (std::streamsize)uSize);
		return true;
	}
	else
		return false;
}

bool KFileDataStream::Reference(char** ppszBuffer, size_t uSize)
{
	if(ppszBuffer)
		*ppszBuffer = nullptr;
	return false;
}