#include "KFileSystem.h"
#include "FileSystem/KNativeFileSystem.h"
#include "FileSystem/KZipFileSystem.h"
#include "FileSystem/KApkFileSystem.h"
#include "FileSystem/KMultiFileSystem.h"

#include "Interface/IKLog.h"

#include <algorithm>
#include <assert.h>

namespace KFileSystem
{
	bool CreateFileManager()
	{
		assert(Manager == nullptr);
		if (Manager == nullptr)
		{
			Manager = IKFileSystemManagerPtr((IKFileSystemManager*)new KFileSystemManager());
		}
		return true;
	}

	bool DestroyFileManager()
	{
		assert(Manager != nullptr);
		if (Manager != nullptr)
		{
			Manager = nullptr;
		}
		return true;
	}

	IKFileSystemManagerPtr Manager = nullptr;

	IKFileSystemPtr CreateFileSystem(FileSystemType type)
	{
		IKFileSystemPtr fileSys = nullptr;
		switch (type)
		{
		case FST_NATIVE:
		{
			fileSys = IKFileSystemPtr(KNEW KNativeFileSystem());
			break;
		}
		case FST_ZIP:
		{
			fileSys = IKFileSystemPtr(KNEW KZipFileSystem());
			break;
		}
		case FST_APK:
		{
			fileSys = IKFileSystemPtr(KNEW KApkFileSystem());
			break;
		}
		case FST_MULTI:
		{
			fileSys = IKFileSystemPtr(KNEW KMultiFileSystem());
			break;
		}
		default:
			assert(false && "Unknown type");
		}
		return fileSys;
	}
}

KFileSystemManager::KFileSystemManager()
{
	ZERO_ARRAY_MEMORY(m_FileSystem);
}

KFileSystemManager::~KFileSystemManager()
{
}

bool KFileSystemManager::Init()
{
	UnInit();
	for (IKFileSystemPtr& system : m_FileSystem)
	{
		ASSERT_RESULT(system);
		system->Init();
	}
	return true;
}

bool KFileSystemManager::UnInit()
{
	for (IKFileSystemPtr& system : m_FileSystem)
	{
		ASSERT_RESULT(system);
		system->UnInit();
	}
	return true;
}

bool KFileSystemManager::SetFileSystem(FileSystemDomain domain, IKFileSystemPtr system)
{
	if (domain != FSD_UNKNOWN)
	{
		assert(system);
		m_FileSystem[domain] = system;
		return true;
	}
	return false;
}

bool KFileSystemManager::UnSetFileSystem(FileSystemDomain domain)
{
	if (domain != FSD_UNKNOWN)
	{
		m_FileSystem[domain] = nullptr;
		return true;
	}
	return false;
}

bool KFileSystemManager::UnSetAllFileSystem()
{
	ZERO_ARRAY_MEMORY(m_FileSystem);
	return true;
}

IKFileSystemPtr KFileSystemManager::GetFileSystem(FileSystemDomain domain)
{
	if (domain != FSD_UNKNOWN)
	{
		return m_FileSystem[domain];
	}
	return nullptr;
}