#pragma once
#include "Interface/IKLog.h"

#include <cstdio>
#include <mutex>

class KLogger : public IKLogger
{
protected:
	bool m_bInit;
	bool m_bLogFile;
	bool m_bLogConsole;
	bool m_bLogTime;
	// 锁的粒度较大 用互斥锁
	std::mutex m_Lock;
	void* m_pConsoleHandle;
	FILE* m_pFile;
	IOLineMode m_eLineMode;

	bool _Log(LogLevel level, const char* pszMessage);
public:
	KLogger();
	virtual ~KLogger();

	virtual bool Init(const char* pFilePath, bool bLogConsole, bool bLogTime, IOLineMode lineMode);
	virtual bool UnInit();
	virtual bool SetLogFile(bool bLogFile);
	virtual bool SetLogConsole(bool bLogConsole);
	virtual bool SetLogTime(bool bLogTime);	
	virtual bool Log(LogLevel level, const char* pszFormat, ...);
	virtual bool LogPrefix(LogLevel level, const char* pszPrefix, const char* pszFormat, ...);
	virtual bool LogSuffix(LogLevel level, const char* pszSuffix, const char* pszFormat, ...);
	virtual bool LogPrefixSuffix(LogLevel level, const char* pszPrefix, const char* pszSuffix, const char* pszFormat, ...);
};