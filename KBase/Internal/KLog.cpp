#include "Internal/KLog.h"
#include "Publish/KPlatform.h"
#include "Publish/KFileTool.h"

#include <ctime>
#include <assert.h>
#include <stdarg.h>

#if defined(_WIN32)
#	include <Windows.h>
#	define SNPRINTF sprintf_s
#	define VSNPRINTF _vsnprintf
#	pragma warning(disable : 4996)
#else
#	include <android/log.h>
#	define SNPRINTF snprintf
#	define VSNPRINTF vsnprintf
#endif

struct LogLevelDesc
{
	LogLevel level;
	const char* pszDesc;
	int nLen;
};

const LogLevelDesc LEVEL_DESC[] =
{
	{LL_NORMAL, "", 0},
	{LL_WARNING, "[WARNING]", sizeof("[WARNING]") - 1},
	{LL_DEBUG, "[DEBUG]", sizeof("[DEBUG]") - 1},
	{LL_ERROR, "[ERROR]", sizeof("[ERROR]") - 1},
	{LL_COUNT, "", 0}
};

static_assert(sizeof(LEVEL_DESC) / sizeof(LEVEL_DESC[0]) == LL_COUNT + 1, "LEVEL_DESC COUNT NOT MATCH TO LL_COUNT");

namespace KLog
{
	bool CreateLogger()
	{
		assert(Logger == nullptr);
		if (!Logger)
		{
			Logger = IKLoggerPtr(KNEW KLogger());
		}
		return true;
	}

	bool DestroyLogger()
	{
		assert(Logger != nullptr);
		if (Logger)
		{
			Logger = nullptr;
		}
		return true;
	}

	IKLoggerPtr Logger = nullptr;
}

KLogger::KLogger()
	: m_bInit(false),
	m_bLogFile(false),
	m_bLogConsole(false),
	m_bLogTime(false),
	m_pConsoleHandle(nullptr),
	m_pFile(nullptr),
	m_eLineMode(ILM_COUNT)
{
}

KLogger::~KLogger()
{
#ifdef _WIN32
	assert(m_pConsoleHandle == NULL);
#endif
	assert(m_pFile == nullptr);
	assert(!m_bInit);
}

bool KLogger::Init(const char* pFilePath, bool bLogConsole, bool bLogTime, IOLineMode mode)
{
	UnInit();
	bool bRet = true;
#ifdef _WIN32
	m_pConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	if(pFilePath)
	{
		m_bLogFile = true;
#if defined(_WIN32)
		fopen_s(&m_pFile, pFilePath, "wb");
#elif defined(__ANDROID__)
		std::string fullPath;
		if(KFileTool::PathJoin(KPlatform::GetExternalDataPath(), pFilePath, fullPath))
		{
			m_pFile = fopen(fullPath.c_str(), "wb");
		}
#else
		static_assert(false && "unsupport platform now");
#endif
		if(!m_pFile)
		{
			bRet = false;
		}
	}
	else
	{
		m_bLogFile = false;
	}

	m_bLogConsole = bLogConsole;
	m_bLogTime = bLogTime;
	m_eLineMode = mode;

	m_bInit = bRet;

	return bRet;
}

bool KLogger::UnInit()
{
#ifdef _WIN32
	m_pConsoleHandle = NULL;
#endif
	if(m_pFile)
	{
		fclose(m_pFile);
		m_pFile = nullptr;
	}

	m_bInit = false;

	return true;
}

bool KLogger::SetLogFile(bool bLogFile)
{
	m_bLogFile = bLogFile;
	return true;
}

bool KLogger::SetLogConsole(bool bLogConsole)
{
	m_bLogConsole = bLogConsole;
	return true;
}

bool KLogger::SetLogTime(bool bLogTime)
{
	m_bLogTime = bLogTime;
	return true;
}

bool KLogger::_Log(LogLevel level, const char* pszMessage)
{
	if(m_bInit && pszMessage && (m_bLogFile || m_bLogConsole))
	{
		size_t uLen = strlen(pszMessage);
		if(uLen > 0)
		{
			bool bLogSuccess = true;
			int nPos = 0;

			char szBuffForTime[256]; szBuffForTime[0] = '\0';
			char szBuffMessage[2048]; szBuffMessage[0] = '\0';

			if(m_bLogTime)
			{
				time_t tmtNow = time(nullptr);
				tm tmNow; 
#ifdef _WIN32
				localtime_s(&tmNow, &tmtNow);
#else
				{
					tm* pTm = localtime(&tmtNow);
					if(pTm)
						tmNow = *pTm;
				}
#endif
				// [YEAR-MON-DAY] <HOUR-MIN-SEC>
				nPos = SNPRINTF(szBuffForTime, sizeof(szBuffForTime),
					"[%d-%02d-%02d] <%02d:%02d:%02d>",
					tmNow.tm_year + 1900,
					tmNow.tm_mon + 1,
					tmNow.tm_mday,
					tmNow.tm_hour,
					tmNow.tm_min,
					tmNow.tm_sec);
				assert(nPos > 0);

				nPos = SNPRINTF(szBuffMessage, sizeof(szBuffMessage), "%s", szBuffForTime);
				assert(nPos > 0);
			}

			if(LEVEL_DESC[level].nLen > 0)
			{
				nPos += SNPRINTF(szBuffMessage + nPos , sizeof(szBuffMessage) - nPos,
					nPos > 0 ? " %s" : "%s",
					LEVEL_DESC[level].pszDesc);
				assert(nPos > 0);
			}

			nPos += SNPRINTF(szBuffMessage + nPos , sizeof(szBuffMessage) - nPos,
				nPos > 0 ? " %s" : "%s",
				pszMessage);
			assert(nPos > 0);

			// 锁的位置摆上来 以保证控制台输出顺序与文件输出顺序一致
			std::lock_guard<decltype(m_Lock)> guard(m_Lock);

			if(m_bLogFile)
			{
				bLogSuccess &= fprintf(m_pFile, "%s%s", szBuffMessage, LINE_DESCS[m_eLineMode].pszLine) > 0;
				fflush(m_pFile);
			}

			if(m_bLogConsole)
			{
#if defined(_WIN32)
				switch (level)
				{
				case LL_WARNING:
					SetConsoleTextAttribute(m_pConsoleHandle, MAKEWORD(0x0E, 0));
					break;
				case LL_ERROR:
					SetConsoleTextAttribute(m_pConsoleHandle, MAKEWORD(0x0C, 0));
					break;
				case LL_NORMAL:
				case LL_DEBUG:
				default:
					break;
				}
#endif
				char szTmpBuff[2048]; szTmpBuff[0] = '\0';
				nPos = SNPRINTF(szTmpBuff, sizeof(szTmpBuff), "[LOG] %s\n", szBuffMessage);
				assert(nPos > 0);
				bLogSuccess &= fprintf(stdout, "%s", szTmpBuff) > 0;
#if defined(_WIN32)
				OutputDebugStringA(szTmpBuff);
				SetConsoleTextAttribute(m_pConsoleHandle, MAKEWORD(0x07, 0));
#elif defined(__ANDROID__)
				switch (level)
				{
				case LL_NORMAL:
					__android_log_print(ANDROID_LOG_INFO, "ANDROID", "%s", szTmpBuff);
					break;
				case LL_WARNING:
					__android_log_print(ANDROID_LOG_WARN, "ANDROID", "%s", szTmpBuff);
					break;
				case LL_DEBUG:
					__android_log_print(ANDROID_LOG_DEBUG, "ANDROID", "%s", szTmpBuff);
					break;
				case LL_ERROR:
					__android_log_print(ANDROID_LOG_ERROR, "ANDROID", "%s", szTmpBuff);
					break;
				default:
					assert(false && "unable to reach");
					break;
				}
#else
				printf("%s", szTmpBuff);
#endif
			}
			return bLogSuccess;
		}
	}
	return false;
}

bool KLogger::Log(LogLevel level, const char* pszFormat, ...)
{
	bool bRet = false;
	va_list list;
	va_start(list, pszFormat);
	char szBuffer[2048]; szBuffer[0] = '\0';
	VSNPRINTF(szBuffer, sizeof(szBuffer), pszFormat, list);
	bRet = _Log(level, szBuffer);
	va_end(list);
	return bRet;
}

bool KLogger::LogPrefix(LogLevel level, const char* pszPrefix, const char* pszFormat, ...)
{
	bool bRet = false;
	int nPos = 0;
	va_list list;
	va_start(list, pszFormat);	
	char szBuffer[2048]; szBuffer[0] = '\0';
	nPos += SNPRINTF(szBuffer, sizeof(szBuffer), "%s ", pszPrefix);
	assert(nPos >= 0);
	VSNPRINTF(szBuffer + nPos, sizeof(szBuffer) - nPos, pszFormat, list);
	bRet = Log(level, szBuffer);
	va_end(list);
	return bRet;
}

bool KLogger::LogSuffix(LogLevel level, const char* pszSuffix, const char* pszFormat, ...)
{
	bool bRet = false;
	int nPos = 0;
	va_list list;
	va_start(list, pszFormat);	
	char szBuffer[2048]; szBuffer[0] = '\0';
	nPos += VSNPRINTF(szBuffer + nPos, sizeof(szBuffer), pszFormat, list);
	assert(nPos >= 0);
	SNPRINTF(szBuffer + nPos, sizeof(szBuffer) - nPos, " %s", pszSuffix);
	bRet = Log(level, szBuffer);
	va_end(list);
	return bRet;
}

bool KLogger::LogPrefixSuffix(LogLevel level, const char* pszPrefix, const char* pszSuffix, const char* pszFormat, ...)
{
	bool bRet = false;
	int nPos = 0;
	va_list list;
	va_start(list, pszFormat);	
	char szBuffer[2048]; szBuffer[0] = '\0';
	nPos += SNPRINTF(szBuffer, sizeof(szBuffer), "%s ", pszPrefix);
	assert(nPos >= 0);
	nPos += VSNPRINTF(szBuffer + nPos, sizeof(szBuffer), pszFormat, list);
	assert(nPos >= 0);
	SNPRINTF(szBuffer + nPos, sizeof(szBuffer) - nPos, " %s", pszSuffix);
	bRet = Log(level, szBuffer);
	va_end(list);
	return bRet;
}