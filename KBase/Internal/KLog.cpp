#include "Internal/KLog.h"

#include <ctime>
#include <assert.h>
#include <stdarg.h>

IKLogPtr GLogger = nullptr;

#ifdef _WIN32
#	include <Windows.h>
#	define VSNPRINTF _vsnprintf
#	pragma warning(disable : 4996)
#else
#	define VSNPRINTF vsnprintf
#endif

struct KLogLevelDesc
{
	LogLevel level;
	const char* pszDesc;
	int nLen;
};

const KLogLevelDesc LEVEL_DESC[] =
{
	{LL_NORMAL, "", 0},
	{LL_WARNING, "[WARNING]", sizeof("[WARNING]") - 1},
	{LL_ERROR, "[ERROR]", sizeof("[ERROR]") - 1},
	{LL_COUNT, "", 0}
};
static_assert(sizeof(LEVEL_DESC) / sizeof(LEVEL_DESC[0]) == LL_COUNT + 1, "LEVEL_DESC COUNT NOT MATCH TO LL_COUNT");

EXPORT_DLL IKLogPtr CreateLog()
{
	return IKLogPtr(new KLog());
}

KLog::KLog()
	: m_bInit(false),
	m_bLogFile(false),
	m_bLogConsole(false),
	m_bLogTime(false),
	m_pConsoleHandle(nullptr),
	m_pFile(nullptr),
	m_eLineMode(ILM_COUNT)
{
#ifdef _WIN32
	m_pConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
}

KLog::~KLog()
{
	UnInit();
}

bool KLog::Init(const char* pFilePath, bool bLogConsole, bool bLogTime, IOLineMode mode)
{
	UnInit();
	bool bRet = true;
	if(pFilePath)
	{
		m_bLogFile = true;
#ifdef _WIN32
		fopen_s(&m_pFile, pFilePath, "wb");
#else
		m_pFile = fopen(pFilePath, "wb");
#endif
		if(m_pFile == nullptr)
			bRet = false;
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

bool KLog::UnInit()
{
	if(m_pFile)
	{
		fclose(m_pFile);
		m_pFile = nullptr;
	}
	return true;
}

bool KLog::SetLogFile(bool bLogFile)
{
	m_bLogFile = bLogFile;
	return true;
}

bool KLog::SetLogConsole(bool bLogConsole)
{
	m_bLogConsole = bLogConsole;
	return true;
}

bool KLog::SetLogTime(bool bLogTime)
{
	m_bLogTime = bLogTime;
	return true;
}

bool KLog::_Log(LogLevel level, const char* pszMessage)
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
				bLogSuccess &= fprintf(m_pFile, "%s%s" , szBuffMessage, LINE_DESCS[m_eLineMode].pszLine) > 0;

			if(m_bLogConsole)
			{
#ifdef _WIN32
				switch (level)
				{
				case LL_WARNING:
					SetConsoleTextAttribute(m_pConsoleHandle, MAKEWORD(0x0E, 0));
					break;
				case LL_ERROR:
					SetConsoleTextAttribute(m_pConsoleHandle, MAKEWORD(0x0C, 0));
					break;
				case LL_NORMAL:
				default:
					break;
				}
#endif
				char szTmpBuff[2048]; szTmpBuff[0] = '\0';
				nPos = SNPRINTF(szTmpBuff, sizeof(szTmpBuff), ">.[LOG] %s\n", szBuffMessage);
				assert(nPos > 0);
				bLogSuccess &= fprintf(stdout, szTmpBuff) > 0;
#ifdef _WIN32
				switch (level)
				{
				case LL_WARNING:
				case LL_ERROR:
					SetConsoleTextAttribute(m_pConsoleHandle, MAKEWORD(0x07, 0));
				default:
					break;
				}
				OutputDebugStringA(szTmpBuff);
#endif
			}
			return bLogSuccess;
		}
	}
	return false;
}

bool KLog::Log(LogLevel level, const char* pszFormat, ...)
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

bool KLog::LogPrefix(LogLevel level, const char* pszPrefix, const char* pszFormat, ...)
{
	bool bRet = false;
	int nPos = 0;
	va_list list;
	va_start(list, pszFormat);	
	char szBuffer[2048]; szBuffer[0] = '\0';
	nPos = SNPRINTF(szBuffer, sizeof(szBuffer), "%s ", pszPrefix);
	assert(nPos >= 0);
	VSNPRINTF(szBuffer + nPos, sizeof(szBuffer) - nPos, pszFormat, list);
	bRet = Log(level, szBuffer);
	va_end(list);
	return bRet;
}

bool KLog::LogSuffix(LogLevel level, const char* pszSuffix, const char* pszFormat, ...)
{
	bool bRet = false;
	int nPos = 0;
	va_list list;
	va_start(list, pszFormat);	
	char szBuffer[2048]; szBuffer[0] = '\0';
	nPos = VSNPRINTF(szBuffer + nPos, sizeof(szBuffer), pszFormat, list);
	assert(nPos >= 0);
	SNPRINTF(szBuffer + nPos, sizeof(szBuffer)  - nPos, " %s", pszSuffix);
	bRet = Log(level, szBuffer);
	va_end(list);
	return bRet;
}