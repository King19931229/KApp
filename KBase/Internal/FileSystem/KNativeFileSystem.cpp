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