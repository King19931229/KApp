#pragma once
#include "IKDataStream.h"
#include <string>
#include <vector>
#include <assert.h>

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

	virtual bool FullPath(const std::string& path, std::string& fullPath) = 0;
	virtual bool RelPath(const std::string& fullPath, std::string& path) = 0;

	virtual bool ListDir(const std::string& subDir, std::vector<std::string>& listdir) = 0;
	virtual bool IsFile(const std::string& name) = 0;
	virtual bool IsDir(const std::string& name) = 0;

	virtual bool AddSubFileSystem(IKFileSystemPtr system, int priority) = 0;
	virtual bool RemoveSubFileSystem(IKFileSystemPtr system) = 0;
	virtual bool RemoveAllSubFileSystem() = 0;

	virtual bool GetAllSubFileSystem(KFileSystemPtrList& list) = 0;

	// TODO Open区分读写
	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret) = 0;
	virtual bool RemoveFile(const std::string& file) = 0;
	virtual bool RemoveDir(const std::string& folder) = 0;
	virtual bool IsFileExist(const std::string& file) = 0;
};

enum FileSystemDomain
{
	FSD_SHADER,
	FSD_RESOURCE,
	FSD_BACKUP,

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

	inline const char* FileSystemTypeToString(FileSystemType format)
	{
#define ENUM(format) case FST_##format: return #format;
		switch (format)
		{
			ENUM(NATIVE);
			ENUM(ZIP);
			ENUM(APK);
			ENUM(MULTI);
		};
		// keep the compiler happy
		assert(false && "should not reach");
		return "UNKNOWN";
#undef ENUM
	}

	inline FileSystemType StringToFileSystemType(const char* str)
	{
#define CMP(enum_string) if (!strcmp(str, #enum_string)) return FST_##enum_string;
		CMP(NATIVE);
		CMP(ZIP);
		CMP(APK);
		CMP(MULTI);
		// keep the compiler happy
		assert(false && "should not reach");
		return FST_NATIVE;
#undef CMP
	}
}