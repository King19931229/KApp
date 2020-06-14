#pragma once
#include "Interface/IKSourceFile.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class KSourceFile : public IKSourceFile
{
protected:
	std::string m_FilePath;
	std::string m_FileDirPath;
	std::string m_FileName;
	std::string m_OriginalSource;
	std::string m_FinalSource;
	IOHookerPtr m_Hooker;

	typedef std::unordered_set<std::string> IncludeFiles;

	struct FileInfo
	{
		FileInfo* pParent;
		IncludeFiles includeFiles;
		FileInfo()
		{
			pParent = nullptr;
		}
	};
	typedef std::unordered_map<std::string, FileInfo> FileInfos;
	FileInfos m_FileInfos;

	struct MacroInfo
	{
		std::string marco;
		std::string value;
	};
	typedef std::vector<MacroInfo> MacroInfos;
	MacroInfos m_MacroInfos;

	bool EarseComment(std::string& out, const std::string& in);
	bool AddMacroDefine(std::string& out, const std::string& in);
	IKDataStreamPtr GetFileData(const std::string &filePath);

	bool Trim(std::string& input);
	bool Parse(std::string& output, const std::string& dir, const std::string& file, FileInfo* pParent);
public:
	KSourceFile();
	~KSourceFile();

	virtual bool Open(const char* pszFilePath);
	virtual bool Reload();
	virtual bool Clear();
	virtual bool SaveAsFile(const char* pszFilePath, bool bUTF8BOM);
	virtual bool SetIOHooker(IOHookerPtr hooker);
	virtual bool AddMacro(const MacroPair& macroPair);
	virtual bool RemoveAllMacro();
	virtual bool GetAllMacro(std::vector<MacroPair>& macros);
	virtual const char* GetFilePath();
	virtual const char* GetFileDirPath();
	virtual const char* GetFileName();
	virtual const char* GetOriginalSource();
	virtual const char* GetFinalSource();
};