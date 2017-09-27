#pragma once
#include "Interface/IKDataStream.h"

namespace KFileTool
{
	EXPORT_DLL bool IsFileExist(const char* pFilePath);
	EXPORT_DLL bool RemoveFile(const char* pFilePath);
	/*
	*@brief 创建目录
	*@param[in] pDir 目录地址
	*@param[in] bRecursive true则会递归创建至子目录 false则仅创建子目录
	*/
	EXPORT_DLL bool CreateFolder(const char* pDir, bool bRecursive);
	/*
	*@brief 删除目录
	*@param[in] pDir 目录地址
	*@param[in] bRecursive true则会递归删除至父目录 false则仅删除子目录
	*/
	EXPORT_DLL bool RemoveFolder(const char* pDir, bool bRecursive);
	EXPORT_DLL bool TrimPath(char* pDestPath, size_t uDestBufferSize, const char* pSrcPath, bool bTolower);
}