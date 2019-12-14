#include "KFileSystem.h"
#include "FileSystem/KNativeFileSystem.h"
#include "FileSystem/KZipFileSystem.h"

#include <algorithm>
#include <assert.h>

IKFileSystemManagerPtr GFileSystemManager = nullptr;

EXPORT_DLL IKFileSystemManagerPtr CreateFileSystemManager()
{
	return IKFileSystemManagerPtr((IKFileSystemManager*)new KFileSystemManager());
}

KFileSystemManager::KFileSystemManager()
{

}

KFileSystemManager::~KFileSystemManager()
{
	assert(m_Queue.empty() && "File system not clear");
}

bool KFileSystemManager::Init()
{
	UnInit();
	return true;
}

bool KFileSystemManager::UnInit()
{
	for(PriorityFileSystem& sys : m_Queue)
	{
		sys.system->UnInit();
	}
	m_Queue.clear();
	return true;
}

bool KFileSystemManager::AddSystem(const char* root, int priority, FileSystemType type)
{
	if(std::find_if(m_Queue.begin(), m_Queue.end(), [=](const PriorityFileSystem& sysInQueue) {	return FileSysEqual(sysInQueue, root, type); })== m_Queue.end())
	{
		IKFileSystemPtr fileSys = nullptr;
		switch (type)
		{
		case FST_NATIVE:
			{
				fileSys = IKFileSystemPtr(new KNativeFileSystem());
				fileSys->Init(root);
				break;
			}
		case FST_ZIP:
			{
				fileSys = IKFileSystemPtr(new KZipFileSystem());
				fileSys->Init(root);
				break;
			}
		default:
			assert(false && "Unknown type");
		}

		if(fileSys)
		{
			PriorityFileSystem newSys = PriorityFileSystem(fileSys, root, type, priority);
			auto insertPos = std::lower_bound(m_Queue.begin(), m_Queue.end(), newSys, [](const PriorityFileSystem& a, const PriorityFileSystem& b) { return FileSysCmp(a, b); });
			m_Queue.insert(insertPos, newSys);
			return true;
		}
	}

	return false;
}

bool KFileSystemManager::RemoveSystem(const char* root, FileSystemType type)
{
	auto it = std::find_if(m_Queue.begin(),
		m_Queue.end(),
		[=](const PriorityFileSystem& sysInQueue)
	{
		return FileSysEqual(sysInQueue, root, type);
	});

	if(it != m_Queue.end())
	{
		it->system->UnInit();
		m_Queue.erase(it);
	}

	return true;
}

IKFileSystemPtr KFileSystemManager::GetFileSystem(const char* root, FileSystemType type)
{
	auto it = std::find_if(m_Queue.begin(),
		m_Queue.end(),
		[=](const PriorityFileSystem& sysInQueue)
	{
		return FileSysEqual(sysInQueue, root, type);
	});

	if(it != m_Queue.end())
	{
		return it->system;
	}

	return nullptr;
}

bool KFileSystemManager::Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret)
{
	for(PriorityFileSystem& sys : m_Queue)
	{
		if(sys.system->Open(file, priorityType, ret))
		{
			return true;
		}
	}
	return false;
}