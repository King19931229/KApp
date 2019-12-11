#pragma once
#include "IKDataStream.h"
#include <string>

struct IKFileSystem;
typedef std::shared_ptr<IKFileSystem> IKFileSystemPtr;

enum FileSystemType
{
	FST_NATIVE,
	MULTI_FILESYSTEM
};

struct IKFileSystem
{
	virtual ~IKFileSystem() {}
	virtual FileSystemType GetType() = 0;

	virtual bool Init(const std::string& root) = 0;
	virtual bool UnInit() = 0;

	virtual bool GetRoot(std::string& root) = 0;

	virtual bool Open(const std::string& file, IKDataStreamPtr& ret) = 0;
	virtual bool IsFileExist(const std::string& file) = 0;
};