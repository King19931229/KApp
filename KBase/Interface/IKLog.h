#pragma once
#include "Interface/IKDataStream.h"
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

	virtual bool Init(const char* pFilePath, bool bLogConsole, bool bLogTime, IOLineMode lineMode) = 0;
	virtual bool UnInit() = 0;
	virtual bool SetLogFile(bool bLogFile) = 0;
	virtual bool SetLogConsole(bool bLogConsole) = 0;
	virtual bool SetLogTime(bool bLogTime) = 0;
	virtual bool Log(LogLevel level, const char* pszFormat, ...) = 0;
	virtual bool LogPrefix(LogLevel level, const char* pszPrefix, const char* pszFormat, ...) = 0;
	virtual bool LogSuffix(LogLevel level, const char* pszSuffix, const char* pszFormat, ...) = 0;
};

#ifdef _WIN32
#	define SNPRINTF sprintf_s
#else
#	define SNPRINTF snprintf
#endif

#define KLOG(pLog, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	SNPRINTF(szSuffix, "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogSuffix(LL_NORMAL, szSuffix, pszFormat, __VA_ARGS__);\
}

#define KLOGW(pLog, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	SNPRINTF(szSuffix, "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogSuffix(LL_WARNING, szSuffix, pszFormat, __VA_ARGS__);\
}

#define KLOGE(pLog, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	SNPRINTF(szSuffix, "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogSuffix(LL_ERROR, szSuffix, pszFormat, __VA_ARGS__);\
}

#define KLOGE_ASSERT(pLog, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	SNPRINTF(szSuffix, "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogSuffix(LL_ERROR, szSuffix, pszFormat, __VA_ARGS__);\
	assert(false);\
}

extern IKLogPtr GLogger;

#define KG_LOG(pszFormat, ...) KLOG(GLogger, pszFormat, __VA_ARGS__)
#define KG_LOGW(pszFormat, ...) KLOGW(GLogger, pszFormat, __VA_ARGS__)
#define KG_LOGE(pszFormat, ...) KLOGE(GLogger, pszFormat, __VA_ARGS__)
#define KG_LOGE_ASSERT(pszFormat, ...) KLOGE_ASSERT(GLogger, pszFormat, __VA_ARGS__)

EXPORT_DLL IKLogPtr CreateLog();