#include "Publish/KFileTool.h"
#include "Publish/KStringUtil.h"

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#pragma warning (disable:4996)
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <assert.h>

#include <string>
#include <algorithm>

#ifdef _WIN32
#define ACCESS(path, mode) _access(path, mode)
#define MKDIR(path) _mkdir(path)
#define RMDIR(path) _rmdir(path)
#else
#define ACCESS(path, mode) access(path, mode)
#define MKDIR(path) mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define RMDIR(path) rmdir(path)
#endif

namespace KFileTool
{
	bool IsPathExist(const std::string& filePath)
	{
		const char* pFilePath = filePath.c_str();
		if(pFilePath && ACCESS(pFilePath, 0) != -1)
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
			if(ACCESS(pFilePath, 0) == -1)
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
					if(ACCESS(curDir.c_str(), 0) != -1)
						continue;
					if(MKDIR(curDir.c_str()) != 0)
						return false;
				}
			}
			if(ACCESS(str.c_str(), 0) != -1)
			{
				return true;
			}
			return MKDIR(str.c_str()) == 0;
		}
		return false;
	}

	bool RemoveFolder(const std::string& folder)
	{
		if(!folder.empty())
		{
			std::string str = folder;
			std::replace(str.begin(), str.end(), '\\', '/');
			if(str.length() > 0 && *str.rbegin() == '/')
				str.erase(str.end() - 1);
			if(ACCESS(str.c_str(), 0) == -1)
			{
				return true;
			}
			return RMDIR(str.c_str()) == 0;
		}
		return false;
	}

	bool IsFile(const std::string& filePath)
	{
#ifdef _WIN32
		WIN32_FIND_DATAA FindFileData;
		FindFirstFileA(filePath.c_str(), &FindFileData);
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
		{
			return true;
		}
		return false;
#else
		return false;
#endif
	}

	bool IsDir(const std::string& filePath)
	{
#ifdef _WIN32
		WIN32_FIND_DATAA FindFileData;
		FindFirstFileA(filePath.c_str(), &FindFileData);
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			return true;
		}
		return false;
#else
		return false;
#endif
	}

	bool ListDir(const std::string& dir, std::vector<std::string>& listdir)
	{
		listdir.clear();
#ifdef _WIN32
		_finddata_t fileDir;
		intptr_t lfDir = 0;
		std::string target = dir + "/*";
		if ((lfDir = _findfirst(target.c_str(), &fileDir)) != -1)
		{
			do
			{
				if (strcmp(fileDir.name, ".") == 0 || strcmp(fileDir.name, "..") == 0)
					continue;
				listdir.push_back(fileDir.name);
			} while (_findnext(lfDir, &fileDir) == 0);
			_findclose(lfDir);
			return true;
		}
		return false;
#else
		return false;
#endif
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

			// remove extra "./"
			if(trimSubPath.substr(0, 2) == "./")
			{
				trimSubPath.erase(0, 2);
			}

			// remove extra "///////"
			if(trimSubPath.at(0) == '/')
			{
				std::string::size_type firstPos = trimSubPath.find_first_not_of('/');
				if(firstPos != std::string::npos)
				{
					trimSubPath = trimSubPath.erase(0, firstPos);
				}
			}

			destPath = trimPath + "/" + trimSubPath;
			// remove extra "./"
			if(destPath.substr(0, 2) == "./")
			{
				destPath.erase(0, 2);
			}

			return true;
		}
		return false;
	}

	bool ReplaceExt(const std::string& path, const std::string& ext, std::string& destPath)
	{
		std::string::size_type pos = path.find_last_of(".");
		if(pos != std::string::npos)
		{
			destPath = path.substr(0, pos) + ext;
		}
		else
		{
			destPath = path + ext;
		}
		return true;
	}

	bool ParentFolder(const std::string& path, std::string& parentFolder)
	{
		if(!path.empty())
		{
			std::string trimPath = path;
			std::replace(trimPath.begin(), trimPath.end(), '\\', '/');
			std::string::size_type lastPos = trimPath.find_last_of("/");

			if(lastPos != std::string::npos)
			{
				parentFolder = path.substr(0, lastPos);
			}
			else
			{
				parentFolder = path;
			}
			return true;
		}
		return false;
	}

	bool CopyFile(const std::string& src, const std::string& dest)
	{
		bool bRet = false;

		std::string destFolder;
		if(ParentFolder(dest, destFolder))
		{
			if(!IsPathExist(destFolder))
			{
				CreateFolder(destFolder, true);
			}

			FILE* srcHandle = fopen(src.c_str(), "rb");
			if(srcHandle)
			{
				long size = 0;

				fseek(srcHandle, 0, SEEK_END);
				size = ftell(srcHandle);
				fseek(srcHandle, 0, SEEK_SET);

				char* buffer = nullptr;
				if (size > 0)
				{
					buffer = new char[size];
					size_t readCount = fread(buffer, 1, size, srcHandle);
					assert(readCount == (size_t)size);
				}

				FILE* destHandle = fopen(dest.c_str(), "wb");
				if(destHandle)
				{
					if(buffer)
					{
						size_t writeCount = fwrite(buffer, 1, (size_t)size, destHandle);
						assert(writeCount == (size_t)size);
					}
					fclose(destHandle);
					bRet = true;
				}

				SAFE_DELETE_ARRAY(buffer);
				fclose(srcHandle);
			}
		}

		return bRet;
	}
}