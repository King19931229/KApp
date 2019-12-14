#pragma once
#include "Interface/IKFileSystem.h"
#include "zip.h"

class KZipFileSystem : public IKFileSystem
{
	zip_t* m_Zip;
	std::string m_Root;
public:
	KZipFileSystem();
	virtual ~KZipFileSystem();
	virtual FileSystemType GetType();

	virtual bool Init(const std::string& root);
	virtual bool UnInit();

	virtual bool GetRoot(std::string& root);

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret);
	virtual bool IsFileExist(const std::string& file);
};