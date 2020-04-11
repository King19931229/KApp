#pragma once
#include "Interface/IKFileSystem.h"
#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#endif

class KApkFileSystem : public IKFileSystem
{
protected:
#ifdef __ANDROID__
	AAssetManager* m_AssetManager;
#endif
	std::string m_Root;
public:
	KApkFileSystem();
	virtual ~KApkFileSystem();
	virtual FileSystemType GetType();

	virtual bool Init();
	virtual bool UnInit();

	virtual bool SetRoot(const std::string& root);
	virtual bool GetRoot(std::string& root);

	virtual bool AddSubFileSystem(IKFileSystemPtr system, int priority) { return false; }
	virtual bool RemoveSubFileSystem(IKFileSystemPtr system) { return false; }
	virtual bool RemoveAllSubFileSystem() { return false; }
	virtual bool GetAllSubFileSystem(KFileSystemPtrList& list) { return false; }

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret);
	virtual bool IsFileExist(const std::string& file);
};