#include "KFileSystem.h"
#include "FileSystem/KNativieFileSystem.h"

#include <assert.h>

KFileSystemManager::KFileSystemManager()
{

}

KFileSystemManager::~KFileSystemManager()
{
	assert(m_FileSys.empty() && "File system not clear");
}

bool KFileSystemManager::Init()
{
	UnInit();
	return true;
}

bool KFileSystemManager::UnInit()
{
	for(auto& pair : m_FileSys)
	{
		IKFileSystemPtr& sys = pair.second;
		sys->UnInit();
	}
	return true;
}

bool KFileSystemManager::AddSystem(const char* root, FileSystemType type)
{
	if(m_FileSys.find(root) == m_FileSys.end())
	{
		switch (type)
		{
		case FST_NATIVE:
			{
				IKFileSystemPtr sys = IKFileSystemPtr(new KNativeFileSystem());
				sys->Init(root);
				m_FileSys[root] = sys;
				return true;
			}
		default:
			assert(false && "Unknown type");
		}
	}
	return false;
}

bool KFileSystemManager::RemoveSystem(const char* root)
{
	auto it = m_FileSys.find(root);
	if(it != m_FileSys.end())
	{
		IKFileSystemPtr& sys = it->second;
		sys->UnInit();
		sys = nullptr;

		m_FileSys.erase(it);
		return true;
	}
	return false;
}

IKFileSystemPtr KFileSystemManager::GetFileSystem(const char* root)
{
	auto it = m_FileSys.find(root);
	if(it != m_FileSys.end())
	{
		return it->second;
	}
	return nullptr;
}