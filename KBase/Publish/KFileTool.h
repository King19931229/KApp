#pragma once
#include "KBase/Interface/IKDataStream.h"
#include <string>
#include <vector>

namespace KFileTool
{
	EXPORT_DLL bool ExecFolder(std::string& execPath);
	EXPORT_DLL bool AbsPath(const std::string& relpath, std::string& absPath);
	EXPORT_DLL bool IsPathExist(const std::string& filePath);

	EXPORT_DLL bool RemoveFile(const std::string& filePath);
	EXPORT_DLL bool CreateFolder(const std::string& folder, bool bRecursive = false);
	EXPORT_DLL bool RemoveFolder(const std::string& folder);

	EXPORT_DLL bool IsFile(const std::string& filePath);
	EXPORT_DLL bool IsDir(const std::string& filePath);
	EXPORT_DLL bool ListDir(const std::string& dir, std::vector<std::string>& listdir);

	EXPORT_DLL bool TrimPath(const std::string& srcPath, std::string& destPath);
	EXPORT_DLL bool PathJoin(const std::string& path, const std::string& subPath, std::string& destPath);
	EXPORT_DLL bool ReplaceExt(const std::string& path, const std::string& ext, std::string& destPath);
	EXPORT_DLL bool SplitExt(const std::string& path, std::string& name, std::string& ext);
	EXPORT_DLL bool ParentFolder(const std::string& path, std::string& parentFolder);
	EXPORT_DLL bool FileName(const std::string& path, std::string& fileName);
	EXPORT_DLL bool FolderName(const std::string& path, std::string& folder);
	EXPORT_DLL bool CopyFile(const std::string& src, const std::string& dest);
	EXPORT_DLL bool CopyFolder(const std::string& srcFolder, const std::string& destFolder);
}