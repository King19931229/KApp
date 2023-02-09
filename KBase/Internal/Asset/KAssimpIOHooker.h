#pragma once
#include "assimp/IOStream.hpp"
#include "assimp/IOSystem.hpp"
#include "Interface/IKFileSystem.h"

class KAssimpIOStream : public Assimp::IOStream
{
protected:
	IKDataStreamPtr m_DataStream;
public:
	KAssimpIOStream(IKDataStreamPtr stream);
	virtual ~KAssimpIOStream();

	virtual size_t Read(void* pvBuffer, size_t pSize, size_t pCount);
    virtual size_t Write(const void* pvBuffer, size_t pSize, size_t pCount);
    virtual aiReturn Seek(size_t pOffset, aiOrigin pOrigin);
    virtual size_t Tell() const;
    virtual size_t FileSize() const;
    virtual void Flush();

	bool Close();
};

class KAssimpIOHooker : public Assimp::IOSystem
{
protected:
	IKFileSystemManager* m_FileSystemManager;
public:
	KAssimpIOHooker(IKFileSystemManager* fileSysMgr);
	virtual ~KAssimpIOHooker();

	virtual bool Exists(const char* pFile) const;
    virtual char getOsSeparator() const;
    virtual Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb");
    virtual void Close(Assimp::IOStream* pFile);
};