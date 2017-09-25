#include "../Publish/KFileTool.h"

#include <io.h>
#include <direct.h>

#include <string>
#include <algorithm>

namespace KFileTool
{
	bool IsFileExist(const char* pFilePath)
	{
		if(pFilePath && _access(pFilePath, 0) != -1)
			return true;
		return false;
	}

	bool RemoveFile(const char* pFilePath)
	{
		if(pFilePath)
		{
			if(_access(pFilePath, 0) == -1)
				return true;
			return remove(pFilePath) == 0;
		}
		return false;
	}

	bool CreateFolder(const char* pDir, bool bRecursive)
	{
		if(pDir)
		{
			std::string str = pDir;
			std::replace(str.begin(), str.end(), '\\', '/');
			if(str.length() > 0 && *str.rbegin() == '/')
				str.erase(str.end() - 1);

			if(bRecursive)
			{
				const char* pTrimPath = str.c_str();
				const char* pCurDir = pTrimPath;
				size_t uLen = str.length();
				for(;pCurDir = strchr(pCurDir, '/'); ++pCurDir)
				{
					std::string curDir = str.substr(0, pCurDir - pTrimPath);
					if(curDir.length() > 0 && *curDir.rbegin() == ':')
						continue;
					if(_access(curDir.c_str(), 0) != -1)
						continue;
					if(_mkdir(curDir.c_str()) != 0)
						return false;
				}
			}
			if(_access(str.c_str(), 0) != -1)
				return true;
			return _mkdir(str.c_str()) == 0;
		}
		return false;
	}

	bool RemoveFolder(const char* pDir, bool bRecursive)
	{
		if(pDir)
		{
			std::string str = pDir;
			std::replace(str.begin(), str.end(), '\\', '/');
			if(str.length() > 0 && *str.rbegin() == '/')
				str.erase(str.end() - 1);

			if(bRecursive)
			{
				std::string::size_type pos = str.find_last_of('/');

				for(;pos != std::string::npos;
					str = str.substr(0, pos), pos = str.find_last_of('/'))
				{
					if(_access(str.c_str(), 0) == -1)
						continue;
					if(_rmdir(str.c_str()) != 0)
						return false;
				}
				return true;
			}
			if(_access(str.c_str(), 0) == -1)
				return true;
			return _rmdir(str.c_str()) == 0;
		}
		return false;
	}

	bool TrimPath(char* pDestPath, size_t uDestBufferSize, const char* pSrcPath, bool bTolower)
	{
		if(pDestPath && pSrcPath && uDestBufferSize > 0)
		{
			std::string str = pSrcPath;

			if(bTolower)
				std::transform(str.begin(), str.end(), str.begin(), tolower);

			std::replace(str.begin(), str.end(), '\\', '/');

			if(str.length() < uDestBufferSize)
			{
#ifdef _WIN32
				strcpy_s(pDestPath, uDestBufferSize, str.c_str());
#else
				strcpy(pDestPath, str.c_str());
#endif
				return true;
			}
		}
		return false;
	}
}