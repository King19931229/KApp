#include "Internal/KLog.h"
#include "Publish/KPlatform.h"
#include "Publish/KFileTool.h"

#include <ctime>
#include <assert.h>
#include <stdarg.h>

#if defined(_WIN32)
#	include <Windows.h>
#	define SNPRINTF sprintf_s
#	define VSNPRINTF vsnprintf
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

			char* szBufferAlloc = nullptr;

			char* szBufferWrite = szBuffMessage;
			size_t bufferSize = sizeof(szBuffMessage);

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
				nPos = SNPRINTF(szBuffForTime, sizeof(szBuffForTime) - 1,
					"[%d-%02d-%02d] <%02d:%02d:%02d>",
					tmNow.tm_year + 1900,
					tmNow.tm_mon + 1,
					tmNow.tm_mday,
					tmNow.tm_hour,
					tmNow.tm_min,
					tmNow.tm_sec);
				assert(nPos > 0);

				nPos = SNPRINTF(szBuffMessage, sizeof(szBuffMessage) - 1, "%s", szBuffForTime);
				assert(nPos > 0);
			}

			if(LEVEL_DESC[level].nLen > 0)
			{
				nPos += SNPRINTF(szBuffMessage + nPos, sizeof(szBuffMessage) - nPos - 1,
					nPos > 0 ? " %s" : "%s",
					LEVEL_DESC[level].pszDesc);
				assert(nPos > 0);
			}

			size_t msgLen = strlen(pszMessage);

			if (msgLen + nPos + 2 >= sizeof(szBuffMessage))
			{
				szBufferAlloc = KNEW char[msgLen + 1 + nPos + 2048];
				memcpy(szBufferAlloc, szBuffMessage, nPos);
				szBufferWrite = szBufferAlloc;
				bufferSize = msgLen + 1 + nPos + 2048;
			}
			else
			{
				szBufferWrite = szBuffMessage;
				bufferSize = sizeof(szBuffMessage);
			}

			nPos += SNPRINTF(szBufferWrite + nPos, bufferSize - nPos,
				nPos > 0 ? " %s" : "%s",
				pszMessage);

			// 锁的位置摆上来 以保证控制台输出顺序与文件输出顺序一致
			std::lock_guard<decltype(m_Lock)> guard(m_Lock);

			if(m_bLogFile)
			{
				bLogSuccess &= fprintf(m_pFile, "%s%s", szBufferWrite, LINE_DESCS[m_eLineMode].pszLine) > 0;
				fflush(m_pFile);
			}

			if(m_bLogConsole)
			{
#if defined(_WIN32)
				switch (level)
				{
				case LL_WARNING:
					SetConsoleTextAttribute(m_pConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED);
					break;
				case LL_ERROR:
					SetConsoleTextAttribute(m_pConsoleHandle, FOREGROUND_INTENSITY | FOREGROUND_RED);
					break;
				case LL_NORMAL:
				case LL_DEBUG:
				default:
					break;
				}
#endif

				char szTmpBuff[2048] = { 0 };
				char* szTmpBufferWrite = nullptr;
				char* szTempBufferAlloc = nullptr;

				if (szBufferAlloc)
				{
					szTempBufferAlloc = KNEW char[bufferSize];
					memset(szTempBufferAlloc, 0, bufferSize);
					szTmpBufferWrite = szTempBufferAlloc;
				}
				else
				{
					szTmpBufferWrite = szTmpBuff;
				}

				nPos = SNPRINTF(szTmpBufferWrite, bufferSize - 1, "[LOG] %s\n", szBufferWrite);
				assert(nPos > 0);
				bLogSuccess &= fprintf(stdout, "%s", szTmpBufferWrite) > 0;
#if defined(_WIN32)
				OutputDebugStringA(szTmpBufferWrite);
				SetConsoleTextAttribute(m_pConsoleHandle, MAKEWORD(0x07, 0));
#elif defined(__ANDROID__)
				switch (level)
				{
				case LL_NORMAL:
					__android_log_print(ANDROID_LOG_INFO, "ANDROID", "%s", szTmpBufferWrite);
					break;
				case LL_WARNING:
					__android_log_print(ANDROID_LOG_WARN, "ANDROID", "%s", szTmpBufferWrite);
					break;
				case LL_DEBUG:
					__android_log_print(ANDROID_LOG_DEBUG, "ANDROID", "%s", szTmpBufferWrite);
					break;
				case LL_ERROR:
					__android_log_print(ANDROID_LOG_ERROR, "ANDROID", "%s", szTmpBufferWrite);
					break;
				default:
					assert(false && "unable to reach");
					break;
				}
#else
				printf("%s", szTmpBufferWrite);
#endif
				SAFE_DELETE_ARRAY(szTempBufferAlloc);
			}
			SAFE_DELETE_ARRAY(szBufferAlloc);
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
	int requireBufferSize = VSNPRINTF(szBuffer, sizeof(szBuffer) - 1, pszFormat, list);
	if (requireBufferSize > sizeof(szBuffer) - 1)
	{
		char *szAllocBuffer = KNEW char[requireBufferSize + 1];
		memset(szAllocBuffer, 0, requireBufferSize + 1);
		VSNPRINTF(szAllocBuffer, requireBufferSize/* + 1 - 1 */, pszFormat, list);
		bRet = _Log(level, szAllocBuffer);
		KDELETE[] szAllocBuffer;
	}
	else
	{
		bRet = _Log(level, szBuffer);
	}

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
	nPos += SNPRINTF(szBuffer, sizeof(szBuffer) - nPos - 1, "%s ", pszPrefix);

	int requireBufferSize = VSNPRINTF(szBuffer + nPos, sizeof(szBuffer) - nPos - 1, pszFormat, list);
	if (requireBufferSize > (int)sizeof(szBuffer) - nPos - 1)
	{
		char *szAllocBuffer = KNEW char[requireBufferSize + 1 + 2048];
		memcpy(szAllocBuffer, szBuffer, nPos);
		VSNPRINTF(szAllocBuffer + nPos, requireBufferSize - nPos + 2048, pszFormat, list);
		bRet = _Log(level, szAllocBuffer);
		KDELETE[] szAllocBuffer;
	}
	else
	{
		bRet = _Log(level, szBuffer);
	}

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
	int requireBufferSize = VSNPRINTF(szBuffer, sizeof(szBuffer) - 1, pszFormat, list);
	if (requireBufferSize > sizeof(szBuffer) - 1)
	{
		char *szAllocBuffer = KNEW char[requireBufferSize + 1 + 2048];
		memset(szAllocBuffer, 0, requireBufferSize + 1);

		nPos = VSNPRINTF(szAllocBuffer, requireBufferSize + 2048, pszFormat, list);
		nPos += SNPRINTF(szAllocBuffer + nPos, requireBufferSize - nPos + 2048, " %s", pszSuffix);

		bRet = _Log(level, szAllocBuffer);
		KDELETE[] szAllocBuffer;
	}
	else
	{
		nPos += requireBufferSize;
		SNPRINTF(szBuffer + nPos, sizeof(szBuffer) - nPos - 1, " %s", pszSuffix);
		bRet = Log(level, szBuffer);
	}

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

	int requireBufferSize = VSNPRINTF(szBuffer + nPos, sizeof(szBuffer) - nPos - 1, pszFormat, list);
	if (requireBufferSize > (int)sizeof(szBuffer) - nPos - 1)
	{
		char *szAllocBuffer = KNEW char[requireBufferSize + 1 + 2048];
		memcpy(szAllocBuffer, szBuffer, nPos);
		nPos += VSNPRINTF(szAllocBuffer + nPos, requireBufferSize - nPos + 2048, pszFormat, list);
		SNPRINTF(szAllocBuffer + nPos, requireBufferSize - nPos + 2048, " %s", pszSuffix);
		bRet = Log(level, szAllocBuffer);
		KDELETE[] szAllocBuffer;
	}
	else
	{
		nPos += requireBufferSize;
		SNPRINTF(szBuffer + nPos, sizeof(szBuffer) - nPos - 1, " %s", pszSuffix);
		bRet = Log(level, szBuffer);
	}
	
	va_end(list);
	return bRet;
}