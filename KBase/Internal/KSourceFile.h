#pragma once
#include "Interface/IKSourceFile.h"

#include <string>
#include <map>
#include <set>

class KSourceFile : public IKSourceFile
{
protected:
	std::string m_FilePath;
	std::string m_FileDirPath;
	std::string m_FileName;
	std::string m_OriginalSource;
	std::string m_FinalSource;

	typedef std::set<std::string> IncludeFiles;

	struct FileInfo
	{
		FileInfo* pParent;
		IncludeFiles includeFiles;
		FileInfo()
		{
			pParent = nullptr;
		}
	};
	typedef std::map<std::string, FileInfo> FileInfos;
	FileInfos m_FileInfos;

	bool EarseComment(std::string& out, const std::string& in);
	IKDataStreamPtr GetFileData(std::string &filePath);

	bool Trim(std::string& input);
	bool Parse(std::string& output, const std::string& dir, const std::string& file, FileInfo* pParent);
public:
	KSourceFile();
	~KSourceFile();

	virtual bool Open(const char* pszFilePath);
	virtual bool Reload();
	virtual bool Clear();
	virtual bool SaveAsFile(const char* pszFilePath, bool bUTF8BOM);
	virtual const char* GetFilePath();
	virtual const char* GetFileDirPath();
	virtual const char* GetFileName();
	virtual const char* GetOriginalSource();
	virtual const char* GetFinalSource();
};