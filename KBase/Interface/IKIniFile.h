#pragma once
#include "IKDataStream.h"

struct IKIniFile;
typedef std::shared_ptr<IKIniFile> IKIniFilePtr;

/**
*文件形式

**Note: 暂时没有支持Trim空格

# 注释
[SECTION0]
KEY0=VALUE0
KEY1=VALUE2
# 覆盖KEY1的VALUE
KEY1=NEW_VALUE2
......
[SECTION1]
......

**/

struct IKIniFile
{
	virtual ~IKIniFile() {}

	virtual bool Open(const char* pszFilePath) = 0;
	virtual bool SaveAsFile(const char* pszFilePath) = 0;

	virtual bool GetBool(const char* pszSection, const char* pszKey, bool *pValue) = 0;
	virtual bool GetInt(const char* pszSection, const char* pszKey, int *pValue) = 0;
	virtual bool GetFloat(const char* pszSection, const char* pszKey, float *pValue) = 0;
	virtual bool GetString(const char* pszSection, const char* pszKey, char* pszStr, size_t uBufferSize) = 0;

	virtual bool SetBool(const char* pszSection, const char* pszKey, bool *pValue) = 0;
	virtual bool SetInt(const char* pszSection, const char* pszKey, int *pValue) = 0;
	virtual bool SetFloat(const char* pszSection, const char* pszKey, float *pValue) = 0;
	virtual bool SetString(const char* pszSection, const char* pszKey, char* pszValue) = 0;
};

EXPORT_DLL IKIniFilePtr GetIniFile();