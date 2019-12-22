#pragma once
#include "KBase/Interface/IKDataStream.h"
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
	virtual bool LogPrefixSuffix(LogLevel level, const char* pszPrefix, const char* pszSuffix, const char* pszFormat, ...) = 0;
};

#ifdef _WIN32
#	define SNPRINTF sprintf_s
#else
#	define SNPRINTF snprintf
#endif

enum LogModule
{
	LM_DEFAULT,
	LM_IO,
	LM_RENDER,
};

static inline const char* LOG_MOUDLE_TO_STR(LogModule module)
{
	switch (module)
	{
#define	LOG_MODULE_CASE(r) case LM_##r: return "["#r"]";
		LOG_MODULE_CASE(DEFAULT);
		LOG_MODULE_CASE(IO);
		LOG_MODULE_CASE(RENDER);
#undef	LOG_MODULE_CASE
	};

	return "";
}

#define KLOG(pLog, module, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	const char* szPrefix = LOG_MOUDLE_TO_STR(module);\
	SNPRINTF(szSuffix, ARRAY_SIZE(szSuffix), "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogPrefixSuffix(LL_NORMAL, szPrefix, szSuffix, pszFormat, __VA_ARGS__);\
}

#define KLOGW(pLog, module, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	const char* szPrefix = LOG_MOUDLE_TO_STR(module);\
	SNPRINTF(szSuffix, ARRAY_SIZE(szSuffix), "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogPrefixSuffix(LL_WARNING, szPrefix, szSuffix, pszFormat, __VA_ARGS__);\
}

#define KLOGE(pLog, module, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	const char* szPrefix = LOG_MOUDLE_TO_STR(module);\
	SNPRINTF(szSuffix, ARRAY_SIZE(szSuffix), "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogPrefixSuffix(LL_ERROR, szPrefix, szSuffix, pszFormat, __VA_ARGS__);\
}

#define KLOGE_ASSERT(pLog, module, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	const char* szPrefix = LOG_MOUDLE_TO_STR(module);\
	SNPRINTF(szSuffix, ARRAY_SIZE(szSuffix), "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogPrefixSuffix(LL_ERROR, szPrefix, szSuffix, pszFormat, __VA_ARGS__);\
	assert(false);\
}

extern IKLogPtr GLogger;

#define KG_LOG(module, pszFormat, ...) KLOG(GLogger, module, pszFormat, __VA_ARGS__)
#define KG_LOGW(module, pszFormat, ...) KLOGW(GLogger, module, pszFormat, __VA_ARGS__)
#define KG_LOGE(module, pszFormat, ...) KLOGE(GLogger, module, pszFormat, __VA_ARGS__)
#define KG_LOGE_ASSERT(module, pszFormat, ...) KLOGE_ASSERT(GLogger, module, pszFormat, __VA_ARGS__)

EXPORT_DLL IKLogPtr CreateLog();