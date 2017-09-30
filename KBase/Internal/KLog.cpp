#include "KLog.h"

#include <ctime>
#include <sstream>

struct KLogLevelDesc
{
	LogLevel level;
	const char* pszDesc;
};

const KLogLevelDesc LEVEL_DESC[] =
{
	{LL_NORMAL, ""},
	{LL_WARNING, "[WARNING]"},
	{LL_ERROR, "[ERROR]"},
	{LL_COUNT, ""}
};
static_assert(sizeof(LEVEL_DESC) / sizeof(LEVEL_DESC[0]) == LL_COUNT + 1, "LEVEL_DESC COUNT NOT MATCH TO LL_COUNT");

IKLogPtr CreateLog()
{
	return IKLogPtr(new KLog());
}

KLog::KLog()
	: m_bInit(false),
	m_bLogFile(false),
	m_bLogConsole(false),
	m_bLogTime(false),
	m_pFile(nullptr)
{
}

KLog::~KLog()
{
	UnInit();
}

bool KLog::Init(const char* pFilePath, bool bLogConsole)
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
	m_bLogTime = true;

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

bool KLog::Log(LogLevel level, const char* pszMessage)
{
	if(pszMessage && (m_bLogFile || m_bLogConsole))
	{
		size_t uLen = strlen(pszMessage);
		if(uLen > 0)
		{
			bool bLogSuccess = true;

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
#ifdef _WIN32
				sprintf_s(szBuffForTime, sizeof(szBuffForTime),
#else
				sprintf(szBuffForTime,
#endif
					"[%d-%02d-%02d] <%02d:%02d:%02d>",
					tmNow.tm_year + 1900,
					tmNow.tm_mon + 1,
					tmNow.tm_mday,
					tmNow.tm_hour,
					tmNow.tm_min,
					tmNow.tm_sec);
			}
#ifdef _WIN32
			sprintf_s(szBuffMessage, sizeof(szBuffMessage),
#else
			sprintf(szBuffMessage,
#endif
				"%s %s %s\n", szBuffForTime, LEVEL_DESC[level].pszDesc, pszMessage);

			if(m_bLogFile)
				bLogSuccess &= fprintf(m_pFile, "%s", szBuffMessage) > 0;
			if(m_bLogConsole)
				bLogSuccess &= fprintf(stdout, "%s", szBuffMessage) > 0;

			return bLogSuccess;
		}
	}
	return false;
}