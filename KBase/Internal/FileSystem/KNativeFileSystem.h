#pragma once
#include "Interface/IKFileSystem.h"

class KNativeFileSystem : public IKFileSystem
{
protected:
	std::string m_Root;
	std::string m_AbsRoot;
public:
	KNativeFileSystem();
	virtual ~KNativeFileSystem();
	virtual FileSystemType GetType();

	virtual bool Init();
	virtual bool UnInit();

	virtual bool SetRoot(const std::string& root);
	virtual bool GetRoot(std::string& root);

	virtual bool FullPath(const std::string& path, std::string& fullPath);
	virtual bool RelPath(const std::string& fullPath, std::string& path);
	virtual bool ListDir(const std::string& subDir, std::vector<std::string>& listdir);
	virtual bool IsFile(const std::string& name);
	virtual bool IsDir(const std::string& name);

	virtual bool AddSubFileSystem(IKFileSystemPtr system, int priority) { return false; }
	virtual bool RemoveSubFileSystem(IKFileSystemPtr system) { return false; }
	virtual bool RemoveAllSubFileSystem() { return false; }
	virtual bool GetAllSubFileSystem(KFileSystemPtrList& list) { return false; }

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret);
	virtual bool RemoveFile(const std::string& file);
	virtual bool RemoveDir(const std::string& folder);
	virtual bool IsFileExist(const std::string& file);
};