#include "KZipFileSystem.h"
#include "Interface/IKDataStream.h"
#include "Interface/IKLog.h"

#include <vector>
#include <assert.h>

KZipFileSystem::KZipFileSystem()
	: m_Zip(nullptr)
{
}

KZipFileSystem::~KZipFileSystem()
{
	assert(m_Zip == nullptr && "zip not uninit");
}

FileSystemType KZipFileSystem::GetType()
{
	return FST_ZIP;
}

bool KZipFileSystem::Init(const std::string& root)
{
	UnInit();
	m_Root = root;
	m_Zip = zip_open(m_Root.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'r');
	return m_Zip != nullptr;
}

bool KZipFileSystem::UnInit()
{
	if(m_Zip != nullptr)
	{
		zip_close(m_Zip);
		m_Zip = nullptr;
	}
	m_Root.clear();
	return true;
}

bool KZipFileSystem::GetRoot(std::string& root)
{
	m_Root = root;
	return true;
}

bool KZipFileSystem::Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret)
{
	if(m_Zip != nullptr)
	{
		ACTION_ON_FAILURE(zip_entry_open(m_Zip, file.c_str()) == 0, return false);

		void *buf = NULL;
		size_t bufsize = 0;

		if(zip_entry_read(m_Zip, &buf, &bufsize) > 0)
		{
			ret = GetDataStream(IT_MEMORY);
			ret->Open(bufsize, IM_READ_WRITE);
			ret->Write(buf, bufsize);
			ret->Seek(0);

			KG_LOG(LM_IO, "[%s] file open in zip [%s]", file.c_str(), m_Root.c_str());
		}

		ACTION_ON_FAILURE(zip_entry_close(m_Zip) == 0, return false);

		return true;
	}
	return false;
}

bool KZipFileSystem::IsFileExist(const std::string& file)
{
	if(m_Zip != nullptr)
	{
		ACTION_ON_FAILURE(zip_entry_open(m_Zip, file.c_str()) == 0, return false);
		ACTION_ON_FAILURE(zip_entry_close(m_Zip) == 0, return false);
		return true;
	}
	return false;
}