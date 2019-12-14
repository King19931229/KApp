#pragma once
#include "Interface/IKFileSystem.h"

class KNativeFileSystem : public IKFileSystem
{
	std::string m_Root;
public:
	KNativeFileSystem();
	virtual ~KNativeFileSystem();
	virtual FileSystemType GetType();

	virtual bool Init(const std::string& root);
	virtual bool UnInit();

	virtual bool GetRoot(std::string& root);

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret);
	virtual bool IsFileExist(const std::string& file);
};