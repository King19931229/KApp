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
	std::string m_AnnotatedSource;
	std::string m_Header;
	std::vector<std::string> m_IncludePath;
	IOHookerPtr m_Hooker;

	struct FileInfo
	{
		// include该文件的文件
		std::unordered_set<FileInfo*> parents;
		// 该文件最终include的文件列表(完全展开)
		std::unordered_set<std::string> includeFiles;
	};
	std::unordered_map<std::string, FileInfo> m_FileInfos;

	struct MacroInfo
	{
		std::string marco;
		std::string value;
	};
	std::vector<MacroInfo> m_MacroInfos;

	struct IncludeSource
	{
		std::string include;
		std::string source;
	};
	std::vector<IncludeSource> m_IncludeSources;

	struct IncludeFile
	{
		std::string include;
		std::function<std::string()> fileReader;
	};
	std::vector<IncludeFile> m_IncludeFiles;

	void AddIncludeFile(FileInfo* info, const std::string& file);

	bool EarseComment(std::string& out, const std::string& in);
	bool AddMacroDefine(std::string& out, const std::string& in);

	IKDataStreamPtr PostProcessDataStream(IKDataStreamPtr input);
	IKDataStreamPtr GetFileData(const std::string &filePath);
	IKDataStreamPtr GetSourceData(const std::string& source);

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
	virtual bool SetHeaderText(const char* text);
	virtual bool UnsetHeaderText();

	virtual bool AddMacro(const MacroPair& macroPair);
	virtual bool RemoveAllMacro();
	virtual bool GetAllMacro(std::vector<MacroPair>& macros);

	virtual bool AddIncludeSource(const IncludeSourcePair& includeSource);
	virtual bool RemoveAllIncludeSource();
	virtual bool GetAllIncludeSource(std::vector<IncludeSourcePair>& includeSource);

	virtual bool AddIncludeFile(const IncludeFilePair& includeFile);
	virtual bool RemoveAllIncludeFile();
	virtual bool GetAllIncludeFile(std::vector<IncludeFilePair>& includeFile);

	virtual const char* GetFilePath();
	virtual const char* GetFileDirPath();
	virtual const char* GetFileName();
	virtual const char* GetOriginalSource();
	virtual const char* GetFinalSource();
	virtual const char* GetAnnotatedSource();
};