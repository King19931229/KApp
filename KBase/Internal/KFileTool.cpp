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

#include <string>
#include <algorithm>
#include <stack>
#include <assert.h>

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
	bool ExecFolder(std::string& execPath)
	{
#ifdef _WIN32
		char szPath[1024] = { 0 };
		if (GetModuleFileNameA(NULL, szPath, sizeof(szPath) - 1))
		{
			execPath = std::string(szPath);
			std::replace(execPath.begin(), execPath.end(), '\\', '/');
			const char* trimEnd = strrchr(execPath.c_str(), '/');
			const char* trimBegin = execPath.c_str();
			execPath = execPath.substr(0, trimEnd - trimBegin);
			TrimPath(execPath, execPath);
			return true;
		}
		return false;
#else
		// TODO
		return false;
#endif
	}

	bool AbsPath(const std::string& relpath, std::string& absPath)
	{
		char szPath[1024] = { 0 };
		if (getcwd(szPath, sizeof(szPath) - 1))
		{
			absPath = std::string(szPath);
			std::replace(absPath.begin(), absPath.end(), '\\', '/');
			absPath = absPath + "/" + relpath;
			TrimPath(absPath, absPath);
			return true;
		}
		return false;
	}

	bool IsPathExist(const std::string& filePath)
	{
		const char* pFilePath = filePath.c_str();
		if(pFilePath && ACCESS(pFilePath, 0) != -1)
		{
			return true;
		}
		return false;
	}

	bool IsWindowStylePath(const std::string& path)
	{
		if (path.length() >= 2 && isalpha(path.at(0)) && path.at(1) == ':')
		{
			return true;
		}
		return false;
	}

	bool IsUnixStylePath(const std::string& path)
	{
		if (path.length() >= 2 && path.substr(0, 2) == "./")
		{
			return true;
		}
		if (path.length() >= 1 && path.substr(0, 1) == "/")
		{
			return true;
		}
		return false;
	}

	bool SimplifyPath(const std::string& path, std::string& destPath)
	{
		// https://www.geeksforgeeks.org/simplify-directory-path-unix-like/
		if (!path.empty())
		{
			// stack to store the file's names. 
			std::stack<std::string> st;

			// temporary string which stores the extracted 
			// directory name or commands("." / "..") 
			// Eg. "/a/b/../." 
			// dir will contain "a", "b", "..", "."; 
			std::string dir;

			// contains resultant simplifies string. 
			std::string res;

			// every string starts from root directory.
#ifndef _WIN32
			if(IsUnixStylePath(path))
				res.append("/");
#endif

			// stores length of input string. 
			size_t len_A = path.length();

			for (size_t i = 0; i < len_A; i++)
			{
				// we will clear the temporary string 
				// every time to accomodate new directory  
				// name or command. 
				dir.clear();

				// skip all the multiple '/' Eg. "/////"" 
				while (path[i] == '/')
					i++;

				// stores directory's name("a", "b" etc.) 
				// or commands("."/"..") into dir 
				while (i < len_A && path[i] != '/')
				{
					dir.push_back(path[i]);
					i++;
				}

				// if dir has ".." just pop the topmost 
				// element if the stack is not empty 
				// otherwise ignore. 
				if (dir.compare("..") == 0)
				{
					if (!st.empty() && st.top() != "..")
						st.pop();
					else
						st.push(dir);
				}

				// if dir has "." then simply continue 
				// with the process. 
				else if (dir.compare(".") == 0)
					continue;

				// pushes if it encounters directory's  
				// name("a", "b"). 
				else if (dir.length() != 0)
					st.push(dir);
			}

			// a temporary stack  (st1) which will contain  
			// the reverse of original stack(st). 
			std::stack<std::string> st1;
			while (!st.empty())
			{
				st1.push(st.top());
				st.pop();
			}

			// the st1 will contain the actual res. 
			while (!st1.empty())
			{
				std::string temp = st1.top();

				// if it's the last element no need 
				// to append "/" 
				if (st1.size() != 1)
					res.append(temp + "/");
				else
					res.append(temp);

				st1.pop();
			}

			destPath = res;

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
//#ifndef _WIN32
			return remove(pFilePath) == 0;
//#else
//			SHFILEOPSTRUCTA lpfileop;
//			lpfileop.hwnd = NULL;
//			lpfileop.wFunc = FO_DELETE;
//			lpfileop.pFrom = filePath.c_str();
//			lpfileop.pTo = NULL;
//			lpfileop.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
//			lpfileop.hNameMappings = NULL;
//			lpfileop.fAnyOperationsAborted = 0;
//			int nok = SHFileOperationA(&lpfileop);
//			return nok == 0;
//#endif
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
//#ifndef _WIN32
			return RMDIR(str.c_str()) == 0;
//#else
//			SHFILEOPSTRUCTA lpfileop;
//			lpfileop.hwnd = NULL;
//			lpfileop.wFunc = FO_DELETE;
//			lpfileop.pFrom = folder.c_str();
//			lpfileop.pTo = NULL;
//			lpfileop.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
//			lpfileop.hNameMappings = NULL;
//			lpfileop.fAnyOperationsAborted = 0;
//			int nok = SHFileOperationA(&lpfileop);
//			return nok == 0;
//#endif
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
		// TODO
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
		// TODO
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
		// TODO
		return false;
#endif
	}

	bool TrimPath(const std::string& srcPath, std::string& destPath)
	{
		if(!srcPath.empty())
		{
			destPath = srcPath;
			std::replace(destPath.begin(), destPath.end(), '\\', '/');
			SimplifyPath(destPath, destPath);
			return true;
		}
		return false;
	}

	bool PathJoin(const std::string& path, const std::string& subPath, std::string& destPath)
	{
		if(!path.empty() && !subPath.empty())
		{
			std::string trimPath;
			TrimPath(path, trimPath);

			std::string trimSubPath;
			TrimPath(subPath, trimSubPath);

			std::string trimJoinPath = trimPath + "/" + trimSubPath;
			TrimPath(trimJoinPath, destPath);

			return true;
		}
		else if (!path.empty())
		{
			std::string trimJoinPath = path;
			TrimPath(trimJoinPath, destPath);
			return true;
		}
		else if (!subPath.empty())
		{
			std::string trimJoinPath = subPath;
			TrimPath(trimJoinPath, destPath);
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

	bool SplitExt(const std::string& path, std::string& name, std::string& ext)
	{
		std::string::size_type pos = path.find_last_of(".");
		if (pos != std::string::npos)
		{
			name = path.substr(0, pos);
			ext = path.substr(pos);
		}
		else
		{
			name = path;
			ext = "";
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

	bool RelativePath(const std::string& fullPath, const std::string& root, std::string& relPath)
	{
		if (!fullPath.empty() && !root.empty())
		{
			std::string trimFullPath = fullPath;
			std::string trimRoot = root;

			std::replace(trimFullPath.begin(), trimFullPath.end(), '\\', '/');
			std::replace(trimRoot.begin(), trimRoot.end(), '\\', '/');
			if (SimplifyPath(trimFullPath, trimFullPath) && SimplifyPath(trimRoot, trimRoot))
			{
				std::vector<std::string> fullSplitRes;
				KStringUtil::Split(trimFullPath, "/", fullSplitRes);

				std::vector<std::string> rootSplitRes;
				KStringUtil::Split(trimRoot, "/", rootSplitRes);

				if (fullSplitRes.size() >= rootSplitRes.size())
				{
					for (size_t i = 0; i < rootSplitRes.size(); ++i)
					{
						if (fullSplitRes[i] != rootSplitRes[i])
							return false;
					}

					if (fullSplitRes.size() == rootSplitRes.size())
					{
						relPath = "";
						return true;
					}
					else
					{
						relPath = "";
						for (size_t i = rootSplitRes.size(); i < fullSplitRes.size(); ++i)
						{
							relPath += fullSplitRes[i];
							if (i != fullSplitRes.size() - 1)
							{
								relPath += "/";
							}
						}
						return true;
					}
				}
			}
		}
		return false;
	}

	bool FileName(const std::string& path, std::string& fileName)
	{
		if (!path.empty())
		{
			std::string trimPath = path;
			std::replace(trimPath.begin(), trimPath.end(), '\\', '/');
			std::string::size_type lastPos = trimPath.find_last_of("/");

			if (lastPos != std::string::npos && lastPos != fileName.length() - 1)
			{
				fileName = path.substr(lastPos + 1);
			}
			else
			{
				fileName = path;
			}
			return true;
		}
		return false;
	}

	bool FolderName(const std::string& path, std::string& folder)
	{
		return FileName(path, folder);
	}

#ifdef CopyFile
#	undef CopyFile
#endif

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
					buffer = KNEW char[size];
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

	bool CopyFolder(const std::string& srcFolder, const std::string& destFolder)
	{
		if (IsDir(srcFolder))
		{
			if (!IsPathExist(destFolder))
			{
				CreateFolder(destFolder, false);
			}

			std::vector<std::string> list;
			ListDir(srcFolder, list);
			for (const std::string& element : list)
			{
				std::string fullSrcPath;
				std::string fullDestPath;

				PathJoin(srcFolder, element, fullSrcPath);
				PathJoin(destFolder, element, fullDestPath);

				if (IsFile(fullSrcPath))
				{
					CopyFile(fullSrcPath, fullDestPath);
				}
				else
				{
					CopyFolder(fullSrcPath, fullDestPath);
				}
			}
			return true;
		}
		return false;
	}
}