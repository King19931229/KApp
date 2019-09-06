#pragma once
#include "Interface/IKDataStream.h"
#include <string>

namespace KFileTool
{
	/*
	*@brief 判断文件是否存在
	*/
	EXPORT_DLL bool IsFileExist(const std::string& filePath);

	/*
	*@brief 删除文件
	*/
	EXPORT_DLL bool RemoveFile(const std::string& filePath);

	/*
	*@brief 创建目录
	*@param[in] bRecursive true则会递归创建至子目录 false则仅创建子目录
	*/
	EXPORT_DLL bool CreateFolder(const std::string& folder, bool bRecursive = true);

	/*
	*@brief 删除目录
	*@param[in] bRecursive true则会递归删除至父目录 false则仅删除子目录
	*/
	EXPORT_DLL bool RemoveFolder(const std::string& folder, bool bRecursive = false);
	/*
	*@brief 
	*@param[in] bTolower true则会转小写
	*/
	EXPORT_DLL bool TrimPath(const std::string& srcPath, std::string& destPath, bool bTolower = false);

	EXPORT_DLL bool PathJoin(const std::string& path, const std::string& subPath, std::string& destPath);
}