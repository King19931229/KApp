#pragma once
#include "IKDataStream.h"

struct IKSourceFile;
typedef std::shared_ptr<IKSourceFile> KSourceFilePtr;

struct IKSourceFile
{
	virtual ~IKSourceFile() {}

	virtual bool Open(const char* pszFilePath) = 0;
	virtual bool Reload() = 0;
	virtual bool Clear() = 0;
	virtual bool SaveAsFile(const char* pszFilePath, bool bUTF8BOM) = 0;
	virtual const char* GetFilePath() = 0;
	virtual const char* GetFileDirPath() = 0;
	virtual const char* GetFileName() = 0;
	virtual const char* GetOriginalSource() = 0;
	virtual const char* GetFinalSource() = 0;
};

EXPORT_DLL KSourceFilePtr GetSourceFile();