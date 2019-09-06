#include "Publish/KProcess.h"
#include "Publish/KStringTool.h"

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

bool KProcess::Wait(const std::string& path, const std::string& args)
{
	if(!path.empty())
	{
#ifndef _WIN32
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
			int nRet = (int)execv(path.c_str(), pData);
		}
		catch(...)
		{
			nRet = -1;
		}
		return nRet != -1;
#else
		SHELLEXECUTEINFOA shExecInfo = {0};		
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
		shExecInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd   = NULL;
		shExecInfo.lpVerb = "open";
		shExecInfo.lpFile = path.c_str();
		shExecInfo.lpParameters = args.c_str();
		shExecInfo.lpDirectory  = NULL;
		shExecInfo.nShow        = SW_SHOW;
		shExecInfo.hInstApp     = NULL;
		if(ShellExecuteExA(&shExecInfo))
		{
			WaitForSingleObject(shExecInfo.hProcess, INFINITE);
			DWORD returnCode = 1;
			GetExitCodeProcess(shExecInfo.hProcess, &returnCode);
			return returnCode == 0;
		}
		return false;
#endif
	}
	return false;
 }