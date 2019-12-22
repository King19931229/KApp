#pragma once
#include "KBase/Interface/IKDataStream.h"
#include <string>

namespace KFileTool
{
	EXPORT_DLL bool IsPathExist(const std::string& filePath);
	EXPORT_DLL bool RemoveFile(const std::string& filePath);
	EXPORT_DLL bool CreateFolder(const std::string& folder, bool bRecursive = false);
	EXPORT_DLL bool RemoveFolder(const std::string& folder);
	EXPORT_DLL bool TrimPath(const std::string& srcPath, std::string& destPath, bool bTolower = false);
	EXPORT_DLL bool PathJoin(const std::string& path, const std::string& subPath, std::string& destPath);
	EXPORT_DLL bool ParentFolder(const std::string& path, std::string& parentFolder);
	EXPORT_DLL bool CopyFile(const std::string& src, const std::string& dest);
}