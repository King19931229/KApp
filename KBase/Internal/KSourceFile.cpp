#include "Internal/KSourceFile.h"
#include "Publish/KStringUtil.h"
#include "Interface/IKLog.h"
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
	m_IncludePath = { "" };
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

static void ReadLongLine(IKDataStreamPtr stream, char* szBuffer, size_t uBufferSize, std::string& line)
{
	line.clear();
	while(!stream->IsEOF())
	{
		size_t uPos = stream->Tell();
		size_t uReadSize = std::min(uBufferSize - 1, stream->GetSize() - uPos);
		if(uReadSize == 0 || !stream->Read(szBuffer, uReadSize))
			break;

		char* pszLineEnd = uReadSize ? (char*)memchr(szBuffer, '\n', uReadSize) : nullptr;
		if(pszLineEnd)
		{
			line.append(szBuffer, pszLineEnd - szBuffer);
			stream->Seek((long)(uPos + (pszLineEnd - szBuffer) + 1));
			break;
		}
		line.append(szBuffer, uReadSize);
	}
	if(!line.empty() && *line.rbegin() == '\r')
		line.erase(line.end() - 1);
};

IKDataStreamPtr KSourceFile::PostProcessDataStream(IKDataStreamPtr input)
{
	std::string rawFileData, fileData;

	IKDataStreamPtr pData = nullptr;
	if (input)
	{
		char szBuffer[1024] = { 0 };
		std::string longLine;

		while (true)
		{
			if (input->ReadLine(szBuffer, sizeof(szBuffer)))
			{
				rawFileData += szBuffer;
				rawFileData += "\n";
				continue;
			}
			if (input->IsEOF())
				break;
			ReadLongLine(input, szBuffer, sizeof(szBuffer), longLine);
			rawFileData += longLine;
			rawFileData += "\n";
		}

		if (!rawFileData.empty() && *rawFileData.rbegin() == '\n')
			rawFileData.erase(rawFileData.end() - 1);

		if (EarseComment(fileData, rawFileData))
		{
			pData = GetDataStream(IT_MEMORY);
			pData->Open(fileData.length() + 1, IM_READ_WRITE);
			pData->Write(fileData.c_str(), fileData.length() + 1);
			pData->Seek(0);
		}

		input->Close();
	}

	return pData;
}

IKDataStreamPtr KSourceFile::GetFileData(const std::string &filePath)
{
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

	pData = PostProcessDataStream(pData);
	
	return pData;
}

IKDataStreamPtr KSourceFile::GetSourceData(const std::string& source)
{
	IKDataStreamPtr pData = nullptr;

	pData = GetDataStream(IT_MEMORY);
	pData->Open(source.length() + 1, IM_READ_WRITE);
	pData->Write(source.c_str(), source.length() + 1);
	pData->Seek(0);

	pData = PostProcessDataStream(pData);

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

void KSourceFile::AddIncludeFile(FileInfo* info, const std::string& file)
{
	if (info)
	{
		info->includeFiles.insert(file);
		for (FileInfo* parent : info->parents)
		{
			AddIncludeFile(parent, file);
		}
	}
}

bool KSourceFile::Parse(std::string& output, const std::string& dir, const std::string& file, FileInfo* pParent)
{
	if(!file.empty())
	{
		std::string filePath = dir + file;

		std::string curFileData;
		std::string curLine;
		std::string includeFile;
		size_t uLineCount = 0;

		IKDataStreamPtr pData = nullptr;
		
		auto itIncludeSource = std::find_if(m_IncludeSources.begin(), m_IncludeSources.end(), [&filePath](IncludeSource& info)
		{
			return info.include == filePath;
		});

		auto itIncludeFile = std::find_if(m_IncludeFiles.begin(), m_IncludeFiles.end(), [&filePath](IncludeFile& info)
		{
			return info.include == filePath;
		});

		// 通过内部指认头文件内容
		if (itIncludeSource != m_IncludeSources.end())
		{
			pData = GetSourceData(itIncludeSource->source);
		}
		else if (itIncludeFile != m_IncludeFiles.end())
		{
			pData = GetSourceData(itIncludeFile->fileReader());
		}
		// 通过传统文件搜索指认头文件内容
		else
		{
			std::string cache;
			if (m_Hooker && m_Hooker->GetCache(filePath.c_str(), cache))
			{
				pData = GetDataStream(IT_MEMORY);
				pData->Open(cache.c_str(), cache.size(), IM_READ);
			}
			else
			{
				pData = GetFileData(filePath);
				if (pData && m_Hooker)
				{
					size_t dataSize = pData->GetSize();
					cache.resize(dataSize);
					pData->Read(&cache[0], dataSize);
					pData->Seek(0);
					m_Hooker->AddCache(filePath.c_str(), cache);
				}
			}
		}

		if(pData)
		{
			FileInfo *pFileInfo = nullptr;

			// 查找或注册文件信息
			{
				auto itFileInfo = m_FileInfos.find(file);
				// 新增此文件信息
				if (itFileInfo != m_FileInfos.end())
				{
					pFileInfo = &itFileInfo->second;
				}
				else
				{
					pFileInfo = &(m_FileInfos.insert({ file, FileInfo() }).first)->second;
				}
			}

			// 处理include信息
			{
				auto itParent = pFileInfo->parents.find(pParent);
				// 一个新的文件include到了此文件
				if (itParent == pFileInfo->parents.end())
				{
					pFileInfo->parents.insert(pParent);
				}
				// 添加此文件到include文件列表里
				AddIncludeFile(pFileInfo, file);
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

			char szBuffer[1024] = { 0 };
			std::string longLine;

			while(true)
			{
				char* szLine = szBuffer;
				if(!pData->ReadLine(szBuffer, sizeof(szBuffer)))
				{
					if(pData->IsEOF())
						break;
					ReadLongLine(pData, szBuffer, sizeof(szBuffer), longLine);
					szLine = &longLine[0];
				}
				if(strstr(szLine, "#include") == szLine)
				{
					char* pszIncludeBeg = nullptr;
					char* pszIncludeEnd = nullptr;
					pszIncludeBeg = strchr(szLine, '"');
					pszIncludeBeg = pszIncludeBeg ? pszIncludeBeg : strchr(szLine, '<');
					pszIncludeEnd = strrchr(szLine, '"');
					pszIncludeEnd = pszIncludeEnd ? pszIncludeEnd : strchr(szLine, '>');
					if(pszIncludeBeg && pszIncludeEnd && pszIncludeEnd > pszIncludeBeg + 1)
					{
						includeFile.assign(pszIncludeBeg + 1, pszIncludeEnd - pszIncludeBeg - 1);
						Trim(includeFile);

						// 检查include是否合理,是否存在环形依赖
						{
							auto it = m_FileInfos.find(includeFile);
							if(it != m_FileInfos.end())
							{
								FileInfo *pIncludeFileInfo = &it->second;
								// include的文件又include了自己
								if(pIncludeFileInfo->includeFiles.find(file) != pIncludeFileInfo->includeFiles.end())
								{
									KG_LOGE(LM_IO, "Find a circular dependency by including file %s in %s", includeFile.c_str(), file.c_str());
									return false;
								}
							}
						}

						std::string includeFileData;

						// 尝试文件自身路径
						if (!Parse(includeFileData, dir, includeFile, pFileInfo))
						{
							bool Success = false;
							// 尝试额外include路径
							for (const std::string& includePath : m_IncludePath)
							{
								if (includePath == dir) continue;
								if (Parse(includeFileData, includePath, includeFile, pFileInfo))
								{
									Success = true;
									break;
								}
							}

							if (!Success)
							{
								std::string error;
								error += "Could not find include file <" + includeFile + "> in <" + file + ">\n";
								error += "\tSearch path:\n";
								error += "\t\t[" + dir + "]";
								for (const std::string& includePath : m_IncludePath)
								{
									if (includePath == dir) continue;
									error += "\n\t\t[" + includePath + "]";
								}
								KG_LOGE(LM_IO, error.c_str());
								return false;
							}
						}
							
						if (!includeFileData.empty())
						{
							curFileData += includeFileData;
						}
					}
					else
					{
						return false;
					}
				}
				else
				{
					curFileData += std::string(szLine) + "\n";
				}

				if (pParent == nullptr)
				{
					m_OriginalSource += std::string(szLine) + "\n";
				}
			}
			if (pParent == nullptr && !curFileData.empty() && *curFileData.rbegin() == '\n')
			{
				curFileData.erase(curFileData.end() - 1);
			}
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
	m_AnnotatedSource.clear();
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
		}
		// 覆盖掉之前的值
		else
		{
			it->value = value;
		}
		return true;
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

bool KSourceFile::AddIncludeSource(const IncludeSourcePair& includeSource)
{
	const std::string& include = std::get<0>(includeSource);
	const std::string& source = std::get<1>(includeSource);
	if (!include.empty())
	{
		auto it = std::find_if(m_IncludeSources.begin(), m_IncludeSources.end(), [include](const IncludeSource& inc)
		{
			return strcmp(inc.include.c_str(), include.c_str()) == 0;
		});

		if (it == m_IncludeSources.end())
		{
			IncludeSource info = { include, source };
			m_IncludeSources.emplace_back(std::move(info));
		}
		// 覆盖掉之前的值
		else
		{
			it->source = source;
		}
		return true;
	}
	return false;
}

bool KSourceFile::RemoveAllIncludeSource()
{
	m_IncludeSources.clear();
	return true;
}

bool KSourceFile::GetAllIncludeSource(std::vector<IncludeSourcePair>& includeSource)
{
	includeSource.clear();
	for (const IncludeSource& includeInfo : m_IncludeSources)
	{
		includeSource.push_back(std::make_tuple(includeInfo.include, includeInfo.source));
	}
	return true;
}

bool KSourceFile::AddIncludeFile(const IncludeFilePair& includeFile)
{
	const std::string& include = std::get<0>(includeFile);
	const std::function<std::string()>& reader = std::get<1>(includeFile);
	if (!include.empty())
	{
		auto it = std::find_if(m_IncludeFiles.begin(), m_IncludeFiles.end(), [include](const IncludeFile& inc)
		{
			return strcmp(inc.include.c_str(), include.c_str()) == 0;
		});

		if (it == m_IncludeFiles.end())
		{
			IncludeFile info = { include, reader };
			m_IncludeFiles.emplace_back(std::move(info));
		}
		// 覆盖掉之前的值
		else
		{
			it->fileReader = reader;
		}
		return true;
	}
	return false;
}

bool KSourceFile::RemoveAllIncludeFile()
{
	m_IncludeFiles.clear();
	return true;
}

bool KSourceFile::GetAllIncludeFile(std::vector<IncludeFilePair>& includeFile)
{
	includeFile.clear();
	for (const IncludeFile& includeInfo : m_IncludeFiles)
	{
		includeFile.push_back(std::make_tuple(includeInfo.include, includeInfo.fileReader));
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
	if (m_AnnotatedSource.empty() && !m_FinalSource.empty())
	{
		std::vector<std::string> splitResult;
		if (KStringUtil::Split(m_FinalSource, "\n", splitResult))
		{
			m_AnnotatedSource.reserve(m_FinalSource.length() + splitResult.size() * 5);
			for (size_t i = 0; i < splitResult.size(); ++i)
			{
				const std::string& line = splitResult[i];
				m_AnnotatedSource += std::to_string(i + 1) + "> " + line + "\n";
			}
		}
	}
	return m_AnnotatedSource.c_str();
}