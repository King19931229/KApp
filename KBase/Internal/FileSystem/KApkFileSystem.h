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

	virtual bool FullPath(const std::string& path, std::string& fullPath) { return false; }
	virtual bool RelPath(const std::string& fullPath, std::string& path) { return false; }
	virtual bool ListDir(const std::string& subDir, std::vector<std::string>& listdir) { return false; }
	virtual bool IsFile(const std::string& name) { return false; }
	virtual bool IsDir(const std::string& name) { return false; }

	virtual bool AddSubFileSystem(IKFileSystemPtr system, int priority) { return false; }
	virtual bool RemoveSubFileSystem(IKFileSystemPtr system) { return false; }
	virtual bool RemoveAllSubFileSystem() { return false; }
	virtual bool GetAllSubFileSystem(KFileSystemPtrList& list) { return false; }

	virtual bool Open(const std::string& file, IOType priorityType, IKDataStreamPtr& ret, KFileInformation* information);
	virtual bool RemoveFile(const std::string& file) { return false; }
	virtual bool RemoveDir(const std::string& folder) { return false; }
	virtual bool IsFileExist(const std::string& file);
};