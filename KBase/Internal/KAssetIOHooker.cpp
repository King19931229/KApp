#include "KAssetIOHooker.h"
#include <assert.h>

// KAssetIOStream
KAssetIOStream::KAssetIOStream(IKDataStreamPtr stream)
	: m_DataStream(stream)
{
	assert(m_DataStream != nullptr && "DataStream must not empty");
}

KAssetIOStream::~KAssetIOStream()
{
	m_DataStream = nullptr;
}

size_t KAssetIOStream::Read(void* pvBuffer, size_t pSize, size_t pCount)
{
	if(m_DataStream->Read(pvBuffer, pSize * pCount))
	{
		return pCount;
	}
	return 0;
}

size_t KAssetIOStream::Write(const void* pvBuffer, size_t pSize, size_t pCount)
{
	if(m_DataStream->Write(pvBuffer, pSize * pCount))
	{
		return pCount;
	}
	return 0;
}

aiReturn KAssetIOStream::Seek(size_t pOffset, aiOrigin pOrigin)
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

size_t KAssetIOStream::Tell() const
{
	return m_DataStream->Tell();
}

size_t KAssetIOStream::FileSize() const
{
	return m_DataStream->GetSize();
}

void KAssetIOStream::Flush()
{
	m_DataStream->Flush();
}

bool KAssetIOStream::Close()
{
	return m_DataStream->Close();
}

// KAssetIOHooker
KAssetIOHooker::KAssetIOHooker(IKFileSystemManager* fileSysMgr)
	: m_FileSystemManager(fileSysMgr)
{
	assert(fileSysMgr != nullptr && "FileSystem must not empty");
}

KAssetIOHooker::~KAssetIOHooker()
{
}

bool KAssetIOHooker::Exists(const char* pFile) const
{
	IKFileSystemPtr system = m_FileSystemManager->GetFileSystem(FSD_RESOURCE);
	if (!(system && system->IsFileExist(pFile)))
	{
		system = m_FileSystemManager->GetFileSystem(FSD_BACKUP);
		if (system && system->IsFileExist(pFile))
		{
			return true;
		}
	}
	return false;
}

char KAssetIOHooker::getOsSeparator() const
{
	return '/';
}

Assimp::IOStream* KAssetIOHooker::Open(const char* pFile, const char* pMode)
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

	Assimp::IOStream* stream = new KAssetIOStream(dataStream);
	return stream;
}

void KAssetIOHooker::Close(Assimp::IOStream* pFile)
{
	KAssetIOStream* stream = static_cast<KAssetIOStream*>(pFile);
	ASSERT_RESULT(stream->Close());
}