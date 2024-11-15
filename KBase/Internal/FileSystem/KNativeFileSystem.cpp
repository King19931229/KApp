#include "KNativeFileSystem.h"
#include "Publish/KFileTool.h"
#include "Publish/KStringUtil.h"
#include "Interface/IKLog.h"

KNativeFileSystem::KNativeFileSystem()
{
}

KNativeFileSystem::~KNativeFileSystem()
{
}

FileSystemType KNativeFileSystem::GetType()
{
	return FST_NATIVE;
}

bool KNativeFileSystem::Init()
{
	return true;
}

bool KNativeFileSystem::UnInit()
{
	return true;
}

bool KNativeFileSystem::SetRoot(const std::string& root)
{
	m_Root = root;
	KFileTool::AbsPath(m_Root, m_AbsRoot);
	return true;
}

bool KNativeFileSystem::GetRoot(std::string& root)
{
	root = m_Root;
	return true;
}

bool KNativeFileSystem::FullPath(const std::string& path, std::string& fullPath)
{
	if (KFileTool::PathJoin(m_AbsRoot, path, fullPath))
	{
		return true;
	}
	return false;
}

bool KNativeFileSystem::RelPath(const std::string& fullPath, std::string& path)
{
	std::string absPath;
	if (KFileTool::TrimPath(fullPath, absPath))
	{
		if (KStringUtil::StartsWith(absPath, m_AbsRoot) && absPath.length() > m_AbsRoot.length())
		{
			path = absPath.substr(m_AbsRoot.length() + 1);
			return true;
		}
	}
	return false;
}

bool KNativeFileSystem::ListDir(const std::string& subDir, std::vector<std::string>& listdir)
{
	std::string fullPath;
	if (KFileTool::PathJoin(m_AbsRoot, subDir, fullPath))
	{
		if (KFileTool::ListDir(fullPath, listdir))
		{
			return true;
		}
	}
	return false;
}

bool KNativeFileSystem::IsFile(const std::string& name)
{
	std::string fullPath;
	if (KFileTool::PathJoin(m_AbsRoot, name, fullPath))
	{
		if (KFileTool::IsFile(fullPath))
		{
			return true;
		}
	}
	return false;
}

bool KNativeFileSystem::IsDir(const std::string& name)
{
	std::string fullPath;
	if (KFileTool::PathJoin(m_AbsRoot, name, fullPath))
	{
		if (KFileTool::IsDir(fullPath))
		{
			return true;
		}
	}
	return false;
}

bool KNativeFileSystem::Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret, KFileInformation* information)
{
	std::string fullPath;
	if(KFileTool::PathJoin(m_AbsRoot, file, fullPath))
	{
		if(KFileTool::IsPathExist(fullPath))
		{
			ret = GetDataStream(priorityType);
			if(ret->Open(fullPath.c_str(), IM_READ))
			{
				if (information)
				{
					information->fullPath = fullPath;
					KFileTool::ParentFolder(information->fullPath, information->parentFolder);
				}
				KG_LOG(LM_IO, "[%s] file open on disk [%s]",  file.c_str(), m_Root.c_str());
				return true;
			}
		}
	}

	ret = nullptr;
	return false;
}

bool KNativeFileSystem::RemoveFile(const std::string& file)
{
	std::string fullPath;
	if (KFileTool::PathJoin(m_AbsRoot, file, fullPath))
	{
		if (KFileTool::IsPathExist(fullPath))
		{
			return KFileTool::RemoveFile(fullPath);
		}
	}
	return false;
}

bool KNativeFileSystem::RemoveDir(const std::string& folder)
{
	std::string fullPath;
	if (KFileTool::PathJoin(m_AbsRoot, folder, fullPath))
	{
		if (KFileTool::IsPathExist(fullPath))
		{
			return KFileTool::RemoveFolder(fullPath);
		}
	}
	return false;
}

bool KNativeFileSystem::IsFileExist(const std::string& file)
{
	std::string fullPath;

	if(KFileTool::PathJoin(m_AbsRoot, file, fullPath))
	{
		return KFileTool::IsPathExist(fullPath);
	}

	return false;
}