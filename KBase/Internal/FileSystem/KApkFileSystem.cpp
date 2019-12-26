#include "KApkFileSystem.h"
#include "Publish/KPlatform.h"
#include "Publish/KFileTool.h"
#include <assert.h>
#include <vector>

KApkFileSystem::KApkFileSystem() 
#ifdef __ANDROID__
	: m_AssetManager(nullptr)
#endif
{

}

KApkFileSystem::~KApkFileSystem()
{
#ifdef __ANDROID__
	assert(m_AssetManager == nullptr && "AssetManager not empty");
#endif
}

FileSystemType KApkFileSystem::GetType()
{
	return FST_APK;
}

bool KApkFileSystem::Init(const std::string& root)
{
#ifdef __ANDROID__
	assert(KPlatform::androidApp != nullptr && "androidApp is null");
	if(KPlatform::androidApp)
	{
		m_AssetManager = KPlatform::androidApp->activity->assetManager;
		m_Root = root;
		return true;
	}
#endif
	return false;
}

bool KApkFileSystem::UnInit()
{
#ifdef __ANDROID__
	m_AssetManager = nullptr;
#endif
	return true;
}

bool KApkFileSystem::GetRoot(std::string& root)
{
	m_Root = root;
	return true;
}

bool KApkFileSystem::Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret)
{
#ifdef __ANDROID__
	if(m_AssetManager)
	{
		std::string fullPath;
		if (KFileTool::PathJoin(m_Root, file, fullPath))
		{
			AAsset *asset = AAssetManager_open(m_AssetManager, fullPath.c_str(), AASSET_MODE_STREAMING);
			if (asset)
			{
				size_t bufsize = (size_t) AAsset_getLength(asset);
				assert(bufsize > 0);

				std::vector<char> buffer;
				buffer.resize(bufsize);
				AAsset_read(asset, buffer.data(), bufsize);

				ret = GetDataStream(IT_MEMORY);
				ret->Open(bufsize, IM_READ_WRITE);
				ret->Write(buffer.data(), buffer.size());
				ret->Seek(0);

				AAsset_close(asset);
				return true;
			}
		}
	}
#endif
	return false;
}

bool KApkFileSystem::IsFileExist(const std::string& file)
{
#ifdef __ANDROID__
	if(m_AssetManager)
	{
		std::string fullPath;
		if (KFileTool::PathJoin(m_Root, file, fullPath))
		{
			AAsset *asset = AAssetManager_open(m_AssetManager, fullPath.c_str(), AASSET_MODE_STREAMING);
			if (asset)
			{
				AAsset_close(asset);
				return true;
			}
		}
	}
#endif
	return false;
}