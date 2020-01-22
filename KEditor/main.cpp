#include "KEditor.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	KEditor w;

	KLog::CreateLogger();
	KLog::Logger->Init("log.txt", true, true, ILM_UNIX);

	KFileSystem::CreateFileManager();
	KFileSystem::Manager->Init();
	KFileSystem::Manager->AddSystem("../Sponza.zip", -1, FST_ZIP);
	KFileSystem::Manager->AddSystem(".", 0, FST_NATIVE);
	KFileSystem::Manager->AddSystem("../", 1, FST_NATIVE);

	KAssetLoaderManager::CreateAssetLoader();
	KCodec::CreateCodecManager();

	w.Init();
	w.show();
	int nResult = a.exec();
	w.UnInit();

	KAssetLoaderManager::DestroyAssetLoader();
	KCodec::DestroyCodecManager();

	KLog::Logger->UnInit();
	KLog::DestroyLogger();
	KFileSystem::Manager->UnInit();
	KFileSystem::DestroyFileManager();

	return nResult;
}
