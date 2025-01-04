#pragma once
#include "KBase/Interface/IKDataStream.h"

#include <vector>
#include <tuple>
#include <string>

struct IKSourceFile;
typedef std::shared_ptr<IKSourceFile> IKSourceFilePtr;

struct IKSourceFile
{
	struct IOHooker
	{
		virtual bool GetCache(const char* pszPath, std::string& data) = 0;
		virtual bool AddCache(const char* pszPath, const std::string& data) = 0;
		virtual void ClearCache() = 0;
		virtual IKDataStreamPtr Open(const char* pszPath) = 0;
	};
	typedef std::shared_ptr<IOHooker> IOHookerPtr;

	virtual ~IKSourceFile() {}

	virtual bool Open(const char* pszFilePath) = 0;
	virtual bool Reload() = 0;
	virtual bool Clear() = 0;
	virtual bool SaveAsFile(const char* pszFilePath, bool bUTF8BOM) = 0;
	virtual bool SetIOHooker(IOHookerPtr hooker) = 0;

	virtual bool SetHeaderText(const char* text) = 0;
	virtual bool UnsetHeaderText() = 0;

	typedef std::tuple<std::string, std::string> MacroPair;
	virtual bool AddMacro(const MacroPair& macroPair) = 0;
	virtual bool RemoveAllMacro() = 0;	
	virtual bool GetAllMacro(std::vector<MacroPair>& macros) = 0;

	typedef std::tuple<std::string, std::string> IncludeSourcePair;
	virtual bool AddIncludeSource(const IncludeSourcePair& includeSource) = 0;
	virtual bool RemoveAllIncludeSource() = 0;
	virtual bool GetAllIncludeSource(std::vector<IncludeSourcePair>& macros) = 0;

	virtual const char* GetFilePath() = 0;
	virtual const char* GetFileDirPath() = 0;
	virtual const char* GetFileName() = 0;
	virtual const char* GetOriginalSource() = 0;
	virtual const char* GetFinalSource() = 0;
	virtual const char* GetAnnotatedSource() = 0;
};

EXPORT_DLL IKSourceFilePtr GetSourceFile();