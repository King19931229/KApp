#pragma once
#include "Interface/IKIniFile.h"

#include <string>
#include <map>

class KIniFile : public IKIniFile
{
protected:
	typedef std::string KeyType;
	typedef std::string ValueType;

	typedef std::map<KeyType, ValueType> KVTable;
	typedef std::map<std::string, KVTable> Section_KVTable;

	Section_KVTable m_Section_KVTable;
	std::string m_FilePath;

	void Clear();
	bool Parse(IKDataStreamPtr pData);

	KVTable* GetKVTable(const char* pszSection);
	std::string GetValueFromKVTable(KVTable* pTable, const char* pszKey);
	std::string GetValue(const char* pszSection, const char* pszKey);

	template<typename Type>
	bool GetValue(const char* pszSection, const char* pszKey, Type* pValue);
	template<typename Type>
	bool WriteValue(char* pszDest, size_t uBufferSize, const Type* pSrc);
public:
	virtual bool Open(const char* pszFilePath);
	virtual bool SaveAsFile(const char* pszFilePath, IOLineMode mode);

	virtual bool GetBool(const char* pszSection, const char* pszKey, bool *pValue);
	virtual bool GetInt(const char* pszSection, const char* pszKey, int *pValue);
	virtual bool GetFloat(const char* pszSection, const char* pszKey, float *pValue);
	virtual bool GetString(const char* pszSection, const char* pszKey, char* pszStr, size_t uBufferSize);

	virtual bool SetBool(const char* pszSection, const char* pszKey, bool *pValue);
	virtual bool SetInt(const char* pszSection, const char* pszKey, int *pValue);
	virtual bool SetFloat(const char* pszSection, const char* pszKey, float *pValue);
	virtual bool SetString(const char* pszSection, const char* pszKey, char* pszValue);
};