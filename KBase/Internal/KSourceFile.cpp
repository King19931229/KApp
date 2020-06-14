#include "Internal/KSourceFile.h"
#include "Publish/KStringUtil.h"
#include <algorithm>
#include <assert.h>

EXPORT_DLL IKSourceFilePtr GetSourceFile()
{
	IKSourceFilePtr pRet(KNEW KSourceFile());
	return pRet;
}

KSourceFile::KSourceFile()
	: m_Hooker(nullptr)
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

bool KSourceFile::EarseComment(std::string& out, const std::string& in)
{
	std::string::size_type pos = 0;
	std::string::size_type tmpPos = 0;
	std::string result;
	out.reserve(in.size());
	while(pos != std::string::npos)
	{
		if(in[pos] == '/')
		{
			if(in[pos + 1] == '/')
			{
				tmpPos = in.find("\n", pos + 2);
				pos = tmpPos;
				continue;
			}
			else if(in[pos + 1] == '*')
			{
				tmpPos = in.find("*/", pos + 2);
				if(tmpPos == std::string::npos)
					return false;
				pos = tmpPos + 2;
				continue;
			}
			else
			{
				result += in[pos++];
				continue;
			}
		}
		tmpPos = in.find("/", pos);
		if(tmpPos != std::string::npos)
			result += in.substr(pos, tmpPos - pos);
		else
			result += in.substr(pos);
		pos = tmpPos;
	}
	out = result;
	return true;
}

IKDataStreamPtr KSourceFile::GetFileData(const std::string &filePath)
{
	char szBuffer[1024] = { 0 };
	std::string rawFileData, fileData;

	IKDataStreamPtr pData = nullptr;
	if (m_Hooker == nullptr)
	{
		pData = GetDataStream(IT_MEMORY);
		if (!pData->Open(filePath.c_str(), IM_READ))
		{
			pData = nullptr;
		}
	}
	else
	{
		pData = m_Hooker->Open(filePath.c_str());
	}

	if (pData)
	{
		while (pData->ReadLine(szBuffer, sizeof(szBuffer)))
		{
			rawFileData += szBuffer;
			rawFileData += "\n";
		}
		if (*rawFileData.rbegin() == '\n')
			rawFileData.erase(rawFileData.end() - 1);
		if (EarseComment(fileData, rawFileData))
		{
			pData->Close();
			pData = GetDataStream(IT_MEMORY);
			pData->Open(fileData.length() + 1, IM_READ_WRITE);
			pData->Write(fileData.c_str(), fileData.length() + 1);
		}
		pData->Seek(0);
	}
	return pData;
}

bool KSourceFile::AddMacroDefine(std::string& out, const std::string& in)
{
	std::string input = in;
	out.clear();
	for (const MacroInfo& macroInfo : m_MacroInfos)
	{
		if (macroInfo.value.empty())
		{
			out += "#define " + macroInfo.marco + "\n";
		}
		else
		{
			out += "#define " + macroInfo.marco + " " + macroInfo.value + "\n";
		}
	}
	out += input;
	return true;
}

bool KSourceFile::Parse(std::string& output, const std::string& dir, const std::string& file, FileInfo* pParent)
{
	if(!(file.empty()))
	{
		std::string filePath = dir + file;

		std::string curFileData;
		std::string curLine;
		std::string includeFile;
		size_t uLineCount = 0;

		char szBuffer[1024] = {0};
		char szInclude[64] = {0};

		IKDataStreamPtr pData = GetFileData(filePath);
		if(pData)
		{
			FileInfo *pFileInfo = nullptr;
			// 处理include信息
			{
				FileInfos::iterator it = m_FileInfos.find(file);
				if(it == m_FileInfos.end())
					it = m_FileInfos.insert(FileInfos::value_type(file, FileInfo())).first;
				pFileInfo = &it->second;
				pFileInfo->pParent = pParent;
				// 顺着这条新路径把include信息加进去
				for(FileInfo* pPtr = pFileInfo; pPtr != nullptr; pPtr = pPtr->pParent)
					pPtr->includeFiles.insert(file);
			}

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
					nLen = (long)(pszIncludeEnd - pszIncludeBeg - 1);
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

						// 检查include是否合理
						{
							FileInfos::iterator it = m_FileInfos.find(includeFile);
							if(it != m_FileInfos.end())
							{
								FileInfo *pIncludeFileInfo = &it->second;
								if(pIncludeFileInfo->includeFiles.find(file) != pIncludeFileInfo->includeFiles.end())
								{
									// 要include的文件包含了自己
									return false;
								}
							}
						}

						std::string includeFileData;
						if(!Parse(includeFileData, dir, includeFile, pFileInfo))
							return false;
						if(!includeFileData.empty())
							curFileData += includeFileData;
					}
				}
				else
				{
					curFileData += std::string(szBuffer) + "\n";
				}

				if(pParent == nullptr)
					m_OriginalSource += std::string(szBuffer) + "\n";
			}
			if(pParent == nullptr && *curFileData.rbegin() == '\n')
				curFileData.erase(curFileData.end() - 1);
			output += curFileData;

			return true;
		}
	}
	return false;
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
		AddMacroDefine(m_FinalSource, "");
		m_FinalSource = m_Header + m_FinalSource;

		if (Parse(m_FinalSource, m_FileDirPath, m_FileName, nullptr))
		{
			m_AnnotatedSource.clear();
			std::vector<std::string> splitResult;
			if (KStringUtil::Split(m_FinalSource, "\n", splitResult))
			{
				m_AnnotatedSource.reserve(m_FinalSource.length() + splitResult.size() * 5);
				for (size_t i = 0; i < splitResult.size(); ++i)
				{
					const std::string& line = splitResult[i];
					m_AnnotatedSource = m_AnnotatedSource + std::to_string(i) + "> " + line + "\n";
				}
			}
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
	m_FileInfos.clear();
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

bool KSourceFile::SetIOHooker(IOHookerPtr hooker)
{
	m_Hooker = hooker;
	return true;
}

bool KSourceFile::SetHeaderText(const char* text)
{
	m_Header = text;
	return true;
}

bool KSourceFile::UnsetHeaderText()
{
	m_Header.clear();
	return true;
}

bool KSourceFile::AddMacro(const MacroPair& macroPair)
{
	const std::string& macro = std::get<0>(macroPair);
	const std::string& value = std::get<1>(macroPair);
	if (!macro.empty())
	{
		auto it = std::find_if(m_MacroInfos.begin(), m_MacroInfos.end(), [macro](const MacroInfo& mac)
		{
			return strcmp(mac.marco.c_str(), macro.c_str()) == 0;
		});

		if (it == m_MacroInfos.end())
		{
			MacroInfo info = { macro, value };
			m_MacroInfos.emplace_back(std::move(info));
			return true;
		}
	}
	return false;
}

bool KSourceFile::RemoveAllMacro()
{
	m_MacroInfos.clear();
	return true;
}

bool KSourceFile::GetAllMacro(std::vector<MacroPair>& macros)
{
	macros.clear();
	for (const MacroInfo& macroInfo : m_MacroInfos)
	{
		macros.push_back(std::make_tuple(macroInfo.marco, macroInfo.value));
	}
	return true;
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

const char* KSourceFile::GetAnnotatedSource()
{
	return m_AnnotatedSource.c_str();
}