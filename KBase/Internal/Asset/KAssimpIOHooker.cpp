#include "KAssimpIOHooker.h"
#include <assert.h>

// KAssimpIOStream
KAssimpIOStream::KAssimpIOStream(IKDataStreamPtr stream)
	: m_DataStream(stream)
{
	assert(m_DataStream != nullptr && "DataStream must not empty");
}

KAssimpIOStream::~KAssimpIOStream()
{
	m_DataStream = nullptr;
}

size_t KAssimpIOStream::Read(void* pvBuffer, size_t pSize, size_t pCount)
{
	if(m_DataStream->Read(pvBuffer, pSize * pCount))
	{
		return pCount;
	}
	return 0;
}

size_t KAssimpIOStream::Write(const void* pvBuffer, size_t pSize, size_t pCount)
{
	if(m_DataStream->Write(pvBuffer, pSize * pCount))
	{
		return pCount;
	}
	return 0;
}

aiReturn KAssimpIOStream::Seek(size_t pOffset, aiOrigin pOrigin)
{
	long seekPos = 0;

	switch (pOrigin)
	{
	case aiOrigin_SET:
		seekPos = (long)pOffset;
		break;
	case aiOrigin_CUR:
		seekPos = (long)(m_DataStream->Tell() + pOffset);
		break;
	case aiOrigin_END:
		seekPos = (long)m_DataStream->GetSize() + (long)pOffset;
		break;
	default:
		assert(false && "never reach");
		break;
	}

	if(m_DataStream->Seek(seekPos) == seekPos)
	{
		return AI_SUCCESS;
	}
	return AI_FAILURE;

}

size_t KAssimpIOStream::Tell() const
{
	return m_DataStream->Tell();
}

size_t KAssimpIOStream::FileSize() const
{
	return m_DataStream->GetSize();
}

void KAssimpIOStream::Flush()
{
	m_DataStream->Flush();
}

bool KAssimpIOStream::Close()
{
	return m_DataStream->Close();
}

// KAssimpIOHooker
KAssimpIOHooker::KAssimpIOHooker(IKFileSystemManager* fileSysMgr)
	: m_FileSystemManager(fileSysMgr)
{
	assert(fileSysMgr != nullptr && "FileSystem must not empty");
}

KAssimpIOHooker::~KAssimpIOHooker()
{
}

bool KAssimpIOHooker::Exists(const char* pFile) const
{
	IKFileSystemPtr system = m_FileSystemManager->GetFileSystem(FSD_RESOURCE);
	if (!(system && system->IsFileExist(pFile)))
	{
		system = m_FileSystemManager->GetFileSystem(FSD_BACKUP);
		if (!(system && system->IsFileExist(pFile)))
		{
			return false;
		}
	}
	return true;
}

char KAssimpIOHooker::getOsSeparator() const
{
	return '/';
}

Assimp::IOStream* KAssimpIOHooker::Open(const char* pFile, const char* pMode)
{
	if (strstr(pMode, "w"))
	{
		assert(false && "write operation not supported now");
		return nullptr;
	}

	IKDataStreamPtr dataStream = nullptr;
	IKFileSystemPtr system = m_FileSystemManager->GetFileSystem(FSD_RESOURCE);

	if (!(system && system->Open(pFile, IT_FILEHANDLE, dataStream)))
	{
		system = m_FileSystemManager->GetFileSystem(FSD_BACKUP);
		if (!(system && system->Open(pFile, IT_FILEHANDLE, dataStream)))
		{
			return nullptr;
		}
	}

	Assimp::IOStream* stream = KNEW KAssimpIOStream(dataStream);
	return stream;
}

void KAssimpIOHooker::Close(Assimp::IOStream* pFile)
{
	KAssimpIOStream* stream = static_cast<KAssimpIOStream*>(pFile);
	ASSERT_RESULT(stream->Close());
}