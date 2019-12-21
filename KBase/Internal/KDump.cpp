#include "Publish/KDump.h"
#include <string>

static std::string gs_szDumpPath;

#ifdef _WIN32

#include <Windows.h>
#include <DbgHelp.h>
#include <tchar.h>
#include <strsafe.h>

// http://blog.csdn.net/whatday/article/details/44857921
//生产DUMP文件
int GenerateMiniDump(HANDLE hFile, PEXCEPTION_POINTERS pExceptionPointers, PCHAR pAppName)
{
	BOOL bOwnDumpFile = FALSE;
	HANDLE hDumpFile = hFile;
	MINIDUMP_EXCEPTION_INFORMATION ExpParam;

	typedef BOOL(WINAPI * MiniDumpWriteDumpT)(
		HANDLE,
		DWORD,
		HANDLE,
		MINIDUMP_TYPE,
		PMINIDUMP_EXCEPTION_INFORMATION,
		PMINIDUMP_USER_STREAM_INFORMATION,
		PMINIDUMP_CALLBACK_INFORMATION
		);

	MiniDumpWriteDumpT pfnMiniDumpWriteDump = NULL;
	HMODULE hDbgHelp = LoadLibrary(_TEXT("DbgHelp.dll"));
	if (hDbgHelp)
		pfnMiniDumpWriteDump = (MiniDumpWriteDumpT)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

	if (pfnMiniDumpWriteDump)
	{
		if (hDumpFile == NULL || hDumpFile == INVALID_HANDLE_VALUE)
		{
			CHAR szPath[MAX_PATH] = { 0 };
			CHAR szFileName[MAX_PATH] = { 0 };
			CHAR* szAppName = pAppName;
			CHAR* szVersion = "v1.0";
			DWORD dwBufferSize = MAX_PATH;
			SYSTEMTIME stLocalTime;

			GetLocalTime(&stLocalTime);

			if(gs_szDumpPath.empty())
				GetTempPathA(dwBufferSize, szPath);
			else
				StringCchPrintfA(szPath, MAX_PATH, "%s", gs_szDumpPath.c_str());

			StringCchPrintfA(szFileName, MAX_PATH, "%s/%s", szPath, szAppName);
			CreateDirectoryA(szFileName, NULL);

			StringCchPrintfA(szFileName, MAX_PATH, "%s%s//%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
				szPath, szAppName, szVersion,
				stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
				stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
				GetCurrentProcessId(), GetCurrentThreadId());
			hDumpFile = CreateFileA(szFileName, GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

			bOwnDumpFile = TRUE;
			OutputDebugStringA(szFileName);
		}

		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			ExpParam.ThreadId = GetCurrentThreadId();
			ExpParam.ExceptionPointers = pExceptionPointers;
			ExpParam.ClientPointers = FALSE;

			pfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
				hDumpFile, MiniDumpWithDataSegs, (pExceptionPointers ? &ExpParam : NULL), NULL, NULL);

			if (bOwnDumpFile)
				CloseHandle(hDumpFile);
		}
	}

	if (hDbgHelp != NULL)
		FreeLibrary(hDbgHelp);

	return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo)
{
	if (IsDebuggerPresent())
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	return GenerateMiniDump(NULL, lpExceptionInfo, "DumpResult");
}

#endif

bool KDump::Init(const char* pszDumpDir)
{
	gs_szDumpPath = pszDumpDir;
#ifdef _WIN32
	SetUnhandledExceptionFilter(ExceptionFilter);
#endif
	return true;
}

bool KDump::UnInit()
{
	gs_szDumpPath.clear();
#ifdef _WIN32
	SetUnhandledExceptionFilter(NULL);
#endif
	return true;
}