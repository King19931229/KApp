#include "IKSourceFile.h"

#include <string>
#include <set>

class KSourceFile : public IKSourceFile
{
protected:
	std::string m_FilePath;
	std::string m_FileDirPath;
	std::string m_FileName;
	std::string m_OriginalSource;
	std::string m_FinalSource;
	std::set<std::string> m_IncludedFiles;

	bool Trim(std::string& input);
	bool Parse(std::string& output, const std::string& dir, const std::string& file, unsigned short uDepth);
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