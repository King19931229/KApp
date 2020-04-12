#include "KNativeFileSystem.h"
#include "Publish/KFileTool.h"
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
	return true;
}

bool KNativeFileSystem::GetRoot(std::string& root)
{
	root = m_Root;
	return true;
}

bool KNativeFileSystem::FullPath(const std::string& subDir, const std::string& name, std::string& fullPath)
{
	std::string fullDir;
	if (KFileTool::PathJoin(m_Root, subDir, fullDir) && KFileTool::PathJoin(fullDir, name, fullPath))
	{
		return true;
	}
	return false;
}

bool KNativeFileSystem::ListDir(const std::string& subDir, std::vector<std::string>& listdir)
{
	std::string fullPath;
	if (KFileTool::PathJoin(m_Root, subDir, fullPath))
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
	if (KFileTool::PathJoin(m_Root, name, fullPath))
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
	if (KFileTool::PathJoin(m_Root, name, fullPath))
	{
		if (KFileTool::IsDir(fullPath))
		{
			return true;
		}
	}
	return false;
}

bool KNativeFileSystem::Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret)
{
	std::string fullPath;
	if(KFileTool::PathJoin(m_Root, file, fullPath))
	{
		if(KFileTool::IsPathExist(fullPath))
		{
			ret = GetDataStream(priorityType);
			if(ret->Open(fullPath.c_str(), IM_READ))
			{
				KG_LOG(LM_IO, "[%s] file open on disk [%s]",  file.c_str(), m_Root.c_str());
				return true;
			}
		}
	}

	ret = nullptr;
	return false;
}

bool KNativeFileSystem::IsFileExist(const std::string& file)
{
	std::string fullPath;

	if(KFileTool::PathJoin(m_Root, file, fullPath))
	{
		return KFileTool::IsPathExist(fullPath);
	}

	return false;
}