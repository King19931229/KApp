#pragma once
#include "Interface/IKFileSystem.h"
#include <vector>

class KFileSystemManager : IKFileSystemManager
{
protected:
	struct PriorityFileSystem
	{
		IKFileSystemPtr system;
		std::string root;
		FileSystemType type;
		int priority;

		PriorityFileSystem(IKFileSystemPtr _sys, const std::string& _root, FileSystemType _type, int _priority)
		{
			system = _sys;
			root = _root;
			type = _type;
			priority = _priority;
		}
	};
	typedef std::vector<PriorityFileSystem> FileSystemQueue;
	FileSystemQueue m_Queue;

	inline static bool FileSysEqual(const PriorityFileSystem &sys, const std::string& root, FileSystemType type)
	{ 
		return sys.root == root && sys.type == type;
	}
	inline static bool FileSysCmp(const PriorityFileSystem &a, const PriorityFileSystem &b)
	{
		return a.priority < b.priority;
	}
public:
	KFileSystemManager();
	virtual ~KFileSystemManager();

	virtual bool Init();
	virtual bool UnInit();

	virtual bool AddSystem(const char* root, int priority, FileSystemType type);
	virtual bool RemoveSystem(const char* root, FileSystemType type);
	virtual IKFileSystemPtr GetFileSystem(const char* root, FileSystemType type);

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret);
	virtual bool IsFileExist(const std::string& file);
};