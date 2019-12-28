#pragma once
#include "KBase/Interface/IKDataStream.h"
#include <memory>

#if defined(_WIN32)
#	define SNPRINTF sprintf_s
#else
#	define SNPRINTF snprintf
#endif

enum LogLevel
{
	LL_NORMAL,
	LL_WARNING,
	LL_DEBUG,
	LL_ERROR,

	LL_COUNT
};

struct IKLogger;
typedef std::shared_ptr<IKLogger> IKLoggerPtr;

struct IKLogger
{
public:
	virtual ~IKLogger() {}

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

#define KLOGD(pLog, module, pszFormat, ...)\
{\
	char szSuffix[256]; szSuffix[0] = '\0';\
	const char* szPrefix = LOG_MOUDLE_TO_STR(module);\
	SNPRINTF(szSuffix, ARRAY_SIZE(szSuffix), "[%s:%d]", __FILE__, __LINE__);\
	pLog->LogPrefixSuffix(LL_DEBUG, szPrefix, szSuffix, pszFormat, __VA_ARGS__);\
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

namespace KLog
{
	extern IKLoggerPtr Logger;
}

#define KG_LOG(module, pszFormat, ...) KLOG(KLog::Logger, module, pszFormat, __VA_ARGS__)
#define KG_LOGW(module, pszFormat, ...) KLOGW(KLog::Logger, module, pszFormat, __VA_ARGS__)
#define KG_LOGD(module, pszFormat, ...) KLOGD(KLog::Logger, module, pszFormat, __VA_ARGS__)
#define KG_LOGE(module, pszFormat, ...) KLOGE(KLog::Logger, module, pszFormat, __VA_ARGS__)
#define KG_LOGE_ASSERT(module, pszFormat, ...) KLOGE_ASSERT(KLog::Logger, module, pszFormat, __VA_ARGS__)

//EXPORT_DLL IKLoggerPtr CreateLogger();