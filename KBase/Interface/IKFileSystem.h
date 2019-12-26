#pragma once
#include "IKDataStream.h"
#include <string>

struct IKFileSystem;
typedef std::shared_ptr<IKFileSystem> IKFileSystemPtr;

struct IKFileSystemManager;
typedef std::shared_ptr<IKFileSystemManager> IKFileSystemManagerPtr;

enum FileSystemType
{
	FST_NATIVE,
	FST_ZIP,
	FST_APK
};

struct IKFileSystem
{
	virtual ~IKFileSystem() {}
	virtual FileSystemType GetType() = 0;

	virtual bool Init(const std::string& root) = 0;
	virtual bool UnInit() = 0;

	virtual bool GetRoot(std::string& root) = 0;

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret) = 0;
	virtual bool IsFileExist(const std::string& file) = 0;
};

struct IKFileSystemManager
{
	virtual ~IKFileSystemManager() {}

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;

	virtual bool AddSystem(const char* root, int priority, FileSystemType type) = 0;
	virtual bool RemoveSystem(const char* root, FileSystemType type) = 0;
	virtual IKFileSystemPtr GetFileSystem(const char* root, FileSystemType type) = 0;

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret) = 0;
};

extern IKFileSystemManagerPtr GFileSystemManager;
EXPORT_DLL IKFileSystemManagerPtr CreateFileSystemManager();