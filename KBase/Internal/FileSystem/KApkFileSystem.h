#pragma once
#include "Interface/IKFileSystem.h"
#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

class KApkFileSystem : public IKFileSystem
{
#ifdef __ANDROID__
	AAssetManager* m_AssetManager;
#endif
	std::string m_Root;
public:
	KApkFileSystem();
	virtual ~KApkFileSystem();
	virtual FileSystemType GetType();

	virtual bool Init(const std::string& root);
	virtual bool UnInit();

	virtual bool GetRoot(std::string& root);

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret);
	virtual bool IsFileExist(const std::string& file);
};