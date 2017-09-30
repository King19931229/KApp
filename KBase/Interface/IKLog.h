#pragma once
#include "Interface/IKConfig.h"
#include <memory>

enum LogLevel
{
	LL_NORMAL,
	LL_WARNING,
	LL_ERROR,

	LL_COUNT
};

struct IKLog;
typedef std::shared_ptr<IKLog> IKLogPtr;

struct IKLog
{
public:
	virtual ~IKLog() {}

	virtual bool Init(const char* pFilePath, bool bLogConsole) = 0;
	virtual bool UnInit() = 0;
	virtual bool SetLogFile(bool bLogFile) = 0;
	virtual bool SetLogConsole(bool bLogConsole) = 0;
	virtual bool SetLogTime(bool bLogTime) = 0;
	virtual bool Log(LogLevel level, const char* pszMessage) = 0;
};

EXPORT_DLL IKLogPtr CreateLog();