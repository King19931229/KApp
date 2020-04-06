#pragma once
#include "Interface/IKFileSystem.h"
#include <vector>

class KFileSystemManager : IKFileSystemManager
{
protected:
	IKFileSystemPtr m_FileSystem[FSD_COUNT];
public:
	KFileSystemManager();
	virtual ~KFileSystemManager();

	virtual bool Init();
	virtual bool UnInit();

	virtual bool SetFileSystem(FileSystemDomain domain, IKFileSystemPtr system);
	virtual bool UnSetFileSystem(FileSystemDomain domain);
	virtual bool UnSetAllFileSystem();
	virtual IKFileSystemPtr GetFileSystem(FileSystemDomain domain);
};