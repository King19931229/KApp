#pragma once
#include "Interface/IKFileSystem.h"
#include <mutex>
#include "zip.h"

class KZipFileSystem : public IKFileSystem
{
protected:
	zip_t* m_Zip;
	std::mutex m_ZipLock;
	std::string m_Root;
public:
	KZipFileSystem();
	virtual ~KZipFileSystem();
	virtual FileSystemType GetType();

	virtual bool Init();
	virtual bool UnInit();

	virtual bool SetRoot(const std::string& root);
	virtual bool GetRoot(std::string& root);

	virtual bool AddSubFileSystem(IKFileSystemPtr system, int priority) { return false; }
	virtual bool RemoveSubFileSystem(IKFileSystemPtr system) { return false; }
	virtual bool RemoveAllSubFileSystem() { return false; }

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret);
	virtual bool IsFileExist(const std::string& file);
};