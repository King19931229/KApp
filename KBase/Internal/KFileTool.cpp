#include "Publish/KFileTool.h"

#include <io.h>
#include <direct.h>

#include <string>
#include <algorithm>

namespace KFileTool
{
	bool IsFileExist(const std::string& filePath)
	{
		const char* pFilePath = filePath.c_str();
		if(pFilePath && _access(pFilePath, 0) != -1)
		{
			return true;
		}
		return false;
	}

	bool RemoveFile(const std::string& filePath)
	{
		const char* pFilePath = filePath.c_str();
		if(pFilePath)
		{
			if(_access(pFilePath, 0) == -1)
			{
				return true;
			}
			return remove(pFilePath) == 0;
		}
		return false;
	}

	bool CreateFolder(const std::string& folder, bool bRecursive)
	{
		if(!folder.empty())
		{
			std::string str = folder;
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
			{
				return true;
			}
			return _mkdir(str.c_str()) == 0;
		}
		return false;
	}

	bool RemoveFolder(const std::string& folder, bool bRecursive)
	{
		if(!folder.empty())
		{
			std::string str = folder;
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
			{
				return true;
			}
			return _rmdir(str.c_str()) == 0;
		}
		return false;
	}

	bool TrimPath(const std::string& srcPath, std::string& destPath, bool bTolower)
	{
		if(!srcPath.empty())
		{
			destPath = srcPath;

			if(bTolower)
			{
				std::transform(destPath.begin(), destPath.end(), destPath.begin(), tolower);
			}

			std::replace(destPath.begin(), destPath.end(), '\\', '/');
			return true;
		}
		return false;
	}

	bool PathJoin(const std::string& path, const std::string& subPath, std::string& destPath)
	{
		if(!path.empty() && !subPath.empty())
		{
			std::string trimPath = path;
			std::replace(trimPath.begin(), trimPath.end(), '\\', '/');

			std::string::size_type lastPos = trimPath.find_last_not_of("/");
			if(lastPos != std::string::npos && lastPos != trimPath.length() - 1)
			{
				trimPath = trimPath.erase(lastPos + 1, trimPath.length() - lastPos);
			}

			std::string trimSubPath = subPath;
			std::replace(trimSubPath.begin(), trimSubPath.end(), '\\', '/');
			if(trimSubPath.at(0) == '/')
			{
				std::string::size_type firstPos = trimSubPath.find_first_not_of('/');
				if(firstPos != std::string::npos)
				{
					trimSubPath = trimSubPath.erase(0, firstPos);
				}
			}

			destPath = trimPath + "/" + trimSubPath;

			return true;
		}
		return false;
	}

}