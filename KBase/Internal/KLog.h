#include "Interface/IKLog.h"
#include <cstdio>

class KLog : public IKLog
{
protected:
	bool m_bInit;
	bool m_bLogFile;
	bool m_bLogConsole;
	bool m_bLogTime;
	FILE* m_pFile;
public:
	KLog();
	virtual ~KLog();

	virtual bool Init(const char* pFilePath, bool bLogConsole);
	virtual bool UnInit();
	virtual bool SetLogFile(bool bLogFile);
	virtual bool SetLogConsole(bool bLogConsole);
	virtual bool SetLogTime(bool bLogTime);
	virtual bool Log(LogLevel level, const char* pszMessage);
};