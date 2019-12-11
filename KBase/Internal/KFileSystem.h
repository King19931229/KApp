#pragma once
#include "Interface/IKFileSystem.h"

#include <map>
#include <mutex>

class KFileSystemManager
{
public:
	typedef std::map<std::string, IKFileSystemPtr> FileSystemMap;
	static FileSystemMap m_FileSys;
public:
	KFileSystemManager();
	~KFileSystemManager();

	static bool Init();
	static bool UnInit();

	static bool AddSystem(const char* root, FileSystemType type);
	static bool RemoveSystem(const char* root);

	static IKFileSystemPtr GetFileSystem(const char* root);
};