#include "Interface/IKLog.h"
#include "Publish/KSpinLock.h"

#include <cstdio>

class KLog : public IKLog
{
protected:
	bool m_bInit;
	bool m_bLogFile;
	bool m_bLogConsole;
	bool m_bLogTime;
	KSpinLock m_SpinLock;
	void* m_pConsoleHandle;
	FILE* m_pFile;
	IOLineMode m_eLineMode;
public:
	KLog();
	virtual ~KLog();

	virtual bool Init(const char* pFilePath, bool bLogConsole, bool bLogTime, IOLineMode lineMode);
	virtual bool UnInit();
	virtual bool SetLogFile(bool bLogFile);
	virtual bool SetLogConsole(bool bLogConsole);
	virtual bool SetLogTime(bool bLogTime);
	virtual bool Log(LogLevel level, const char* pszMessage);
	virtual bool LogFormat(LogLevel level, const char* pszFormat, ...);
};