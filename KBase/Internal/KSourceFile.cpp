#include "KSourceFile.h"
#include <algorithm>
#include <assert.h>

IKSourceFilePtr GetSourceFile()
{
	IKSourceFilePtr pRet(new KSourceFile());
	return pRet;
}

KSourceFile::KSourceFile()
{

}

KSourceFile::~KSourceFile()
{

}

bool KSourceFile::Trim(std::string& input)
{
	std::transform(input.begin(), input.end(), input.begin(), tolower);
	std::replace(input.begin(), input.end(), '\\', '/');
	return true;
}

bool KSourceFile::Parse(std::string& output, const std::string& dir, const std::string& file, unsigned short uDepth)
{
	if(!(dir.empty() || file.empty()))
	{
		IKDataStreamPtr pData = GetDataStream(IT_MEMORY);
		std::string filePath = dir + file;

		std::string curFileData;
		std::string curLine;
		std::string includeFile;
		size_t uLineCount = 0;

		char szBuffer[1024] = {0};
		char szInclude[64] = {0};
		if(pData->Open(filePath.c_str(), IM_READ))
		{
			m_IncludedFiles.insert(file);
			if(pData->GetSize() > 3)
			{
				char szBufer[] = {0, 0, 0};
				const unsigned char szBOM[] = {0xEF, 0xBB, 0xBF};
				if(pData->Read(szBufer, 3))
				{
					if(memcmp(szBufer, szBOM, 3) != 0)
						pData->Seek(0);
				}
			}

			while(pData->ReadLine(szBuffer, sizeof(szBuffer)))
			{
				if(strstr(szBuffer, "#include") == szBuffer)
				{
					char* pszIncludeBeg = nullptr, *pszIncludeEnd = nullptr;
					long nLen = 0;
					pszIncludeBeg = strchr(szBuffer, '"');
					pszIncludeEnd = strrchr(szBuffer, '"');
					nLen = (long)pszIncludeEnd - (long)pszIncludeBeg - 1;
					if(pszIncludeBeg && pszIncludeEnd && nLen > 0)
					{
#ifndef _WIN32
						strncpy(szInclude, pszIncludeBeg + 1, nLen);
#else
						strncpy_s(szInclude, sizeof(szInclude), pszIncludeBeg + 1, nLen);
#endif
						szInclude[nLen] = '\0';
						includeFile = szInclude;
						Trim(includeFile);
						if(m_IncludedFiles.find(includeFile) != m_IncludedFiles.end())
							return false;
						std::string includeFileData;
						if(!Parse(includeFileData, dir, includeFile, uDepth + 1))
							return false;
						if(!includeFileData.empty())
							curFileData += includeFileData;
					}
				}
				else if(strstr(szBuffer, "#pragma once") == szBuffer)
				{
					if(m_IncludedFiles.find(file) != m_IncludedFiles.end())
						return true;
				}
				else
				{
					curFileData += std::string(szBuffer) + "\n";
				}

				if(uDepth == 0)
					m_OriginalSource += std::string(szBuffer) + "\n";
			}
			if(uDepth == 0 && *curFileData.rbegin() == '\n')
				curFileData.erase(curFileData.end() - 1);
			output += curFileData;

			return true;
		}
	}
	return false;
}

bool KSourceFile::EarseComments()
{
	std::string::size_type pos = 0;
	std::string::size_type tmpPos = 0;
	std::string result;
	result.reserve(m_FinalSource.size());
	while(pos != std::string::npos)
	{
		if(m_FinalSource[pos] == '/')
		{
			if(m_FinalSource[pos + 1] == '/')
			{
				tmpPos = m_FinalSource.find("\n", pos + 2);
				pos = tmpPos;
				continue;
			}
			else if(m_FinalSource[pos + 1] == '*')
			{
				tmpPos = m_FinalSource.find("*/", pos + 2);
				if(tmpPos == std::string::npos)
					return false;
				pos = tmpPos + 2;
				continue;
			}
			else
			{
				assert(false);
				return false;
			}
		}
		else
		{
			tmpPos = m_FinalSource.find("/", pos);
			if(tmpPos != std::string::npos)
				result += m_FinalSource.substr(pos, tmpPos - pos);
			else
				result += m_FinalSource.substr(pos);
			pos = tmpPos;
		}
	}
	m_FinalSource = result;
	return true;
}

bool KSourceFile::Open(const char* pszFilePath)
{
	Clear();
	if(pszFilePath)
	{
		m_FilePath = pszFilePath;
		Trim(m_FilePath);

		std::string::size_type uPos = m_FilePath.find_last_of("/\\");

		if(uPos != std::string::npos)
		{
			m_FileDirPath = m_FilePath.substr(0, uPos + 1);
			if(uPos != m_FilePath.length() - 1)
				m_FileName = m_FilePath.substr(uPos + 1);
			else
				m_FileName.clear();
		}
		else
		{
			m_FileDirPath.clear();
			m_FileName = pszFilePath;
		}
		Trim(m_FileDirPath);
		Trim(m_FileName);
		if(Parse(m_FinalSource, m_FileDirPath, m_FileName, 0))
		{
			if(EarseComments())
				return true;
		}
		Clear();
	}
	return false;
}

bool KSourceFile::Reload()
{
	std::string lastPath = m_FilePath;
	if(!lastPath.empty())
		return Open(lastPath.c_str());
	else
		return false;
}

bool KSourceFile::Clear()
{
	m_FilePath.clear();
	m_FileDirPath.clear();
	m_FileName.clear();
	m_OriginalSource.clear();
	m_FinalSource.clear();
	m_IncludedFiles.clear();
	return true;
}

bool KSourceFile::SaveAsFile(const char* pszFilePath, bool bUTF8BOM)
{
	if(pszFilePath && !m_FinalSource.empty())
	{
		IKDataStreamPtr ptr = GetDataStream(IT_STREAM);
		if(ptr->Open(pszFilePath, IM_WRITE))
		{
			const unsigned char szBOM[] = {0xEF, 0xBB, 0xBF};
			if(bUTF8BOM && !ptr->Write((const char*)szBOM, 3))
				return false;
			if(ptr->Write(m_FinalSource.c_str(), m_FinalSource.size()))
				return true;
		}
	}
	return false;
}

const char* KSourceFile::GetFilePath()
{
	return m_FilePath.c_str();
}

const char* KSourceFile::GetFileDirPath()
{
	return m_FileDirPath.c_str();
}

const char* KSourceFile::GetFileName()
{
	return m_FileName.c_str();
}

const char* KSourceFile::GetOriginalSource()
{
	return m_OriginalSource.c_str();
}

const char* KSourceFile::GetFinalSource()
{
	return m_FinalSource.c_str();
}