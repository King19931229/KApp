#pragma once
#include "Interface/IKFileSystem.h"
#include <vector>

class KMultiFileSystem : public IKFileSystem
{
protected:
	struct PriorityFileSystem
	{
		IKFileSystemPtr system;
		int priority;

		PriorityFileSystem(IKFileSystemPtr _sys, int _priority)
		{
			system = _sys;
			priority = _priority;
		}
	};

	inline static bool FileSysCmp(const PriorityFileSystem &a, const PriorityFileSystem &b)
	{
		return a.priority < b.priority;
	}

	typedef std::vector<PriorityFileSystem> FileSystemQueue;
	FileSystemQueue m_Queue;
public:
	KMultiFileSystem();
	virtual ~KMultiFileSystem();
	virtual FileSystemType GetType();

	virtual bool Init();
	virtual bool UnInit();

	virtual bool SetRoot(const std::string& root) { return false; }
	virtual bool GetRoot(std::string& root) { return false; }

	virtual bool AddSubFileSystem(IKFileSystemPtr system, int priority);
	virtual bool RemoveSubFileSystem(IKFileSystemPtr system);
	virtual bool RemoveAllSubFileSystem();

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret);
	virtual bool IsFileExist(const std::string& file);
};