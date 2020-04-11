#pragma once
#include "IKDataStream.h"
#include <string>
#include <vector>

struct IKFileSystem;
typedef std::shared_ptr<IKFileSystem> IKFileSystemPtr;

struct IKFileSystemManager;
typedef std::unique_ptr<IKFileSystemManager> IKFileSystemManagerPtr;

typedef std::vector<IKFileSystemPtr> KFileSystemPtrList;

enum FileSystemType
{
	FST_NATIVE,
	FST_ZIP,
	FST_APK,
	FST_MULTI
};

struct IKFileSystem
{
	virtual ~IKFileSystem() {}
	virtual FileSystemType GetType() = 0;

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;

	virtual bool SetRoot(const std::string& root) = 0;
	virtual bool GetRoot(std::string& root) = 0;

	virtual bool AddSubFileSystem(IKFileSystemPtr system, int priority) = 0;
	virtual bool RemoveSubFileSystem(IKFileSystemPtr system) = 0;
	virtual bool RemoveAllSubFileSystem() = 0;

	virtual bool GetAllSubFileSystem(KFileSystemPtrList& list) = 0;

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret) = 0;
	virtual bool IsFileExist(const std::string& file) = 0;
};

enum FileSystemDomain
{
	FSD_SHADER,
	FSD_RESOURCE,

	FSD_COUNT,
	FSD_UNKNOWN = FSD_COUNT
};

struct IKFileSystemManager
{
	virtual ~IKFileSystemManager() {}

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;

	virtual bool SetFileSystem(FileSystemDomain domain, IKFileSystemPtr system) = 0;
	virtual bool UnSetFileSystem(FileSystemDomain domain) = 0;
	virtual bool UnSetAllFileSystem() = 0;
	virtual IKFileSystemPtr GetFileSystem(FileSystemDomain domain) = 0;
};

namespace KFileSystem
{
	extern EXPORT_DLL bool CreateFileManager();
	extern EXPORT_DLL bool DestroyFileManager();
	extern EXPORT_DLL IKFileSystemManagerPtr Manager;
	extern EXPORT_DLL IKFileSystemPtr CreateFileSystem(FileSystemType type);
}