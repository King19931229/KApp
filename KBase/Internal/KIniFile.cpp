#include "Internal/KIniFile.h"
#include <sstream>

IKIniFilePtr GetIniFile()
{
	return IKIniFilePtr(new KIniFile());
}

void KIniFile::Clear()
{
	m_Section_KVTable.clear();
}

bool KIniFile::Parse(IKDataStreamPtr pData)
{
	char szBuffer[256] = {0};
	char* pEqualPos = nullptr;

	std::string line;
	std::string section;
	std::string key;
	std::string value;	

	std::string::size_type pos;
	KVTable *pKVTable = nullptr;

	while(pData->ReadLine(szBuffer, sizeof(szBuffer)))
	{
		line = szBuffer;
		if(!line.empty() && *line.begin() == '#')
			continue;
		if(*line.begin() == '[' && *line.rbegin() ==  ']')
		{
			if(line.length() < 3)
				return false;
			section = line.substr(1, line.length() - 2);

			if(m_Section_KVTable.find(section) == m_Section_KVTable.end())
			{
				pKVTable = &m_Section_KVTable.insert(Section_KVTable::value_type(section, KVTable())).first->second;
			}
		}
		else
		{
			if(!pKVTable)
				return false;
			pos = line.find_first_of('=');
			if(pos == std::string::npos)
				return false;
			if(line.find_last_of('=') != pos)
				return false;

			key = line.substr(0, pos);
			value = line.substr(pos + 1, line.length() - 1 - pos);
			if(key.empty() || value.empty())
				return false;

			pKVTable->insert(KVTable::value_type(key, value));
		}
	}
	return true;
}

KIniFile::KVTable* KIniFile::GetKVTable(const char* pszSection)
{
	KVTable* pRet = nullptr;
	if(pszSection)
	{
		Section_KVTable::iterator it = m_Section_KVTable.find(pszSection);
		if(it != m_Section_KVTable.end())
			pRet = &it->second;
	}
	return pRet;
}

std::string KIniFile::GetValueFromKVTable(KVTable* pTable, const char* pszKey)
{
	std::string value;
	if(pTable && pszKey)
	{
		KVTable::iterator it = pTable->find(pszKey);
		if(it != pTable->end())
			value = it->second;
	}
	return value;
}

std::string KIniFile::GetValue(const char* pszSection, const char* pszKey)
{
	std::string value;
	if(pszKey)
	{
		KVTable* pTable = GetKVTable(pszSection);	
		if(pTable)
			value = GetValueFromKVTable(pTable, pszKey);
	}
	return value;
}

bool KIniFile::Open(const char* pszFilePath)
{
	Clear();
	IKDataStreamPtr pData = GetDataStream(IT_MEMORY);
	if(pData->Open(pszFilePath, IM_READ))
		return Parse(pData);
	return false;
}

bool KIniFile::SaveAsFile(const char* pszFilePath, IOLineMode mode)
{
	if(pszFilePath)
	{
		IKDataStreamPtr pData = GetDataStream(IT_FILEHANDLE);
		if(pData->Open(pszFilePath, IM_WRITE))
		{
			std::string finalStr;

			for(auto itSec = m_Section_KVTable.begin(), itSecEnd = m_Section_KVTable.end();
				itSec != itSecEnd; ++itSec)
			{
				finalStr += "[" + itSec->first + "]" + LINE_DESCS[mode].pszLine;
				for(auto it = itSec->second.begin(), itEnd = itSec->second.end();
					it != itEnd; ++it)
					finalStr += it->first + "=" + it->second + LINE_DESCS[mode].pszLine;
			}

			if(finalStr.length() > 0)
				finalStr.erase(finalStr.end() - LINE_DESCS[mode].uLen, finalStr.end());

			return pData->Write(finalStr.c_str(), finalStr.length());
		}
	}
	return false;
}

template<typename Type>
bool KIniFile::GetValue(const char* pszSection, const char* pszKey, Type* pValue)
{
	std::string value = GetValue(pszSection, pszKey);
	if(!value.empty())
	{
		std::stringstream ss;
		ss.str(value);
		ss >> *pValue;
		return true;
	}
	return false;
}

bool KIniFile::GetBool(const char* pszSection, const char* pszKey, bool *pValue)
{
	return GetValue(pszSection, pszKey, pValue);
}

bool KIniFile::GetInt(const char* pszSection, const char* pszKey, int *pValue)
{
	return GetValue(pszSection, pszKey, pValue);
}

bool KIniFile::GetFloat(const char* pszSection, const char* pszKey, float *pValue)
{
	return GetValue(pszSection, pszKey, pValue);
}

bool KIniFile::GetString(const char* pszSection, const char* pszKey, char* pszStr, size_t uBufferSize)
{
	std::string value = GetValue(pszSection, pszKey);
	if(!value.empty())
	{
		if(uBufferSize > value.length())
		{
#ifdef _WIN32
			strcpy_s(pszStr, uBufferSize, value.c_str());
#else
			strcpy(pszStr, value.c_str());
#endif
			return true;
		}
	}
	return false;
}

bool KIniFile::SetBool(const char* pszSection, const char* pszKey, bool *pValue)
{
	return false;
}

bool KIniFile::SetInt(const char* pszSection, const char* pszKey, int *pValue)
{
	return false;
}

bool KIniFile::SetFloat(const char* pszSection, const char* pszKey, float *pValue)
{
	return false;
}

bool KIniFile::SetString(const char* pszSection, const char* pszKey, char* pszValue)
{
	return false;
}