#include "KMultiFileSystem.h"
#include "KBase/Interface/IKLog.h"
#include <algorithm>
#include <assert.h>

KMultiFileSystem::KMultiFileSystem()
{
}

KMultiFileSystem::~KMultiFileSystem()
{
}

FileSystemType KMultiFileSystem::GetType()
{
	return FST_MULTI;
}

bool KMultiFileSystem::Init()
{
	for (PriorityFileSystem& sys : m_Queue)
	{
		ASSERT_RESULT(sys.system);
		sys.system->Init();
	}
	return true;
}

bool KMultiFileSystem::UnInit()
{
	for (PriorityFileSystem& sys : m_Queue)
	{
		ASSERT_RESULT(sys.system);
		sys.system->UnInit();
	}
	return true;
}

bool KMultiFileSystem::AddSubFileSystem(IKFileSystemPtr system, int priority)
{
	if (system)
	{
		if (std::find_if(m_Queue.begin(), m_Queue.end(), [=](const PriorityFileSystem& sysInQueue) { return sysInQueue.system == system; }) == m_Queue.end())
		{
			PriorityFileSystem newSys = PriorityFileSystem(system, priority);
			auto insertPos = std::lower_bound(m_Queue.begin(), m_Queue.end(), newSys, [](const PriorityFileSystem& a, const PriorityFileSystem& b) { return FileSysCmp(a, b); });
			m_Queue.insert(insertPos, newSys);
			return true;
		}
	}
	return false;
}

bool KMultiFileSystem::RemoveSubFileSystem(IKFileSystemPtr system)
{
	auto it = std::find_if(m_Queue.begin(),
		m_Queue.end(),
		[=](const PriorityFileSystem& sysInQueue)
	{
		return sysInQueue.system == system;
	});

	if (it != m_Queue.end())
	{
		m_Queue.erase(it);
	}

	return true;
}

bool KMultiFileSystem::RemoveAllSubFileSystem()
{
	m_Queue.clear();
	return true;
}

bool KMultiFileSystem::GetAllSubFileSystem(KFileSystemPtrList& list)
{
	list.clear();
	list.reserve(m_Queue.size());
	for (PriorityFileSystem& sys : m_Queue)
	{
		list.push_back(sys.system);
	}
	return true;
}

bool KMultiFileSystem::Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret, KFileInformation* information)
{
	for (PriorityFileSystem& sys : m_Queue)
	{
		if (sys.system->Open(file, priorityType, ret, information))
		{
			return true;
		}
	}
	KG_LOGE(LM_IO, "file [%s] can't to be found by mulit file system", file.c_str());
	return false;
}

bool KMultiFileSystem::RemoveFile(const std::string& file)
{
	for (PriorityFileSystem& sys : m_Queue)
	{
		sys.system->RemoveFile(file);
	}
	return true;
}

bool KMultiFileSystem::RemoveDir(const std::string& folder)
{
	for (PriorityFileSystem& sys : m_Queue)
	{
		sys.system->RemoveDir(folder);
	}
	return true;
}

bool KMultiFileSystem::IsFileExist(const std::string& file)
{
	for (PriorityFileSystem& sys : m_Queue)
	{
		if (sys.system->IsFileExist(file))
		{
			return true;
		}
	}
	KG_LOGE(LM_IO, "file [%s] can't to be found by mulit file system", file.c_str());
	return false;
}