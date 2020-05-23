#include "Publish/KSystem.h"
#include "Publish/KStringParser.h"

#ifdef _WIN32
#include <process.h>
#include <Windows.h>
#include <tchar.h>
#else
#include <unistd.h>
#endif

#include <stdarg.h>
#include <errno.h>
#include <vector>

bool KSystem::WaitProcess(const std::string& path, const std::string& args, const std::string& workingDirectory, std::string& output)
{
	if(!path.empty())
	{
#ifndef _WIN32
		// TODO workingDirectory

		std::vector<const char*> c_argsList;
		std::vector<std::string> argList;
		if(!args.empty())
		{
			std::string::size_type offset = 0;
			while(offset < args.length())
			{
				std::string::size_type nonSpace = args.find_first_not_of(' ', offset);
				std::string::size_type space = args.find_first_of(' ', offset);
				// both found
				if(nonSpace != std::string::npos && space != std::string::npos)
				{
					// starts with space
					if(nonSpace > space)
					{
						offset = nonSpace;
						continue;
					}
					argList.push_back(args.substr(nonSpace, space - nonSpace));
					offset = space + 1;
				}
				// non space not found. exit
				else if(nonSpace == std::string::npos)
				{
					break;
				}
				// space not found
				else
				{
					argList.push_back(args.substr(nonSpace));
					break;
				}
			}

			for(const std::string& string : argList)
			{
				c_argsList.push_back(string.c_str());
			}
		}
		c_argsList.push_back(NULL);
		const char** pData = c_argsList.data();
		int nRet = -1;
		try
		{
#ifdef _WIN32
			int nRet = (int)execv(path.c_str(), pData);
#else
			int nRet = (int)execv(path.c_str(), (char* const*)*pData);
#endif
		}
		catch(...)
		{
			nRet = -1;
		}
		return nRet != -1;
#else

#define PIPE_BUFFER_SIZE 1024
#define COMMAND_BUFFER_SIZE 1024
		// https://blog.csdn.net/explorer114/article/details/79951972
		// https://bbs.csdn.net/topics/481942
		SECURITY_ATTRIBUTES saOutPipe;
		STARTUPINFOA startupInfo;
		PROCESS_INFORMATION processInfo;
		DWORD dwErrorCode;
		HANDLE hPipeRead = NULL;
		HANDLE hPipeWrite = NULL;
		char szPipeOut[PIPE_BUFFER_SIZE] = {0};

		saOutPipe.nLength = sizeof(SECURITY_ATTRIBUTES);
		saOutPipe.lpSecurityDescriptor = NULL;
		saOutPipe.bInheritHandle = TRUE;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
		ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

#define CLEAN_UP_PIPE()\
		{\
				if (hPipeRead)\
				{\
					CloseHandle(hPipeRead);\
					hPipeRead = NULL;\
				}\
				if (hPipeWrite)\
				{\
					CloseHandle(hPipeWrite);\
					hPipeWrite = NULL;\
				}\
		}

		if (!CreatePipe(&hPipeRead, &hPipeWrite, &saOutPipe, PIPE_BUFFER_SIZE))
		{
			dwErrorCode = GetLastError();
			return false;
		}

		std::string fullCommand = path + " " + args;
		CHAR szCommandLine[COMMAND_BUFFER_SIZE] = {0};
		if(fullCommand.length() + 1 > COMMAND_BUFFER_SIZE)
		{
			CLEAN_UP_PIPE();
			return false;
		}
		strcpy(szCommandLine, fullCommand.c_str());

		DWORD dwReadLen = 0;
		DWORD dwStdLen = 0;
		startupInfo.cb = sizeof(STARTUPINFO);
		startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		startupInfo.hStdOutput = hPipeWrite;
		startupInfo.hStdError = hPipeWrite;
		startupInfo.wShowWindow = SW_HIDE;

		if (!CreateProcessA(NULL,
			szCommandLine,
			NULL, NULL, TRUE, 0, NULL,
			!workingDirectory.empty() ? workingDirectory.c_str() : NULL,
			&startupInfo, &processInfo))
		{
			dwErrorCode = GetLastError();
			CLEAN_UP_PIPE();
			return false;
		}

#define CLEAN_UP_PROCESS()\
		{\
			if (processInfo.hThread)\
			{\
				CloseHandle(processInfo.hThread);\
				processInfo.hThread = NULL;\
			}\
			if (processInfo.hProcess)\
			{\
				CloseHandle(processInfo.hProcess);\
				processInfo.hProcess = NULL;\
			}\
		}

		DWORD returnCode = 1;
		while(true)
		{
			// 预览管道中数据的内容
			if (!PeekNamedPipe(hPipeRead, NULL, 0, NULL, &dwReadLen, NULL))
			{
				dwErrorCode = GetLastError();
				CLEAN_UP_PIPE();
				CLEAN_UP_PROCESS();
				return false;
			}
			else if(dwReadLen > 0)
			{
				ZeroMemory(szPipeOut, sizeof(szPipeOut));
				// 读取管道中的数据
				DWORD dwRestLen = dwReadLen;
				while(dwRestLen > 0)
				{
					DWORD dwReadCount = min(dwRestLen, PIPE_BUFFER_SIZE - 1);
					if (ReadFile(hPipeRead, szPipeOut, dwReadCount, &dwStdLen, NULL))
					{
						szPipeOut[dwStdLen] = 0;
						output += szPipeOut;
						dwRestLen -= dwStdLen;
					}
					else
					{
						dwErrorCode = GetLastError();
						CLEAN_UP_PIPE();
						CLEAN_UP_PROCESS();
						return false;
					}
				}
			}
			GetExitCodeProcess(processInfo.hProcess, &returnCode);
			if(returnCode != STILL_ACTIVE)
				break;
		}
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		GetExitCodeProcess(processInfo.hProcess, &returnCode);
		CLEAN_UP_PIPE();
		CLEAN_UP_PROCESS();

		return returnCode == 0;
#endif
	}
	return false;
 }