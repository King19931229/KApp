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

	KLog::Logger->Init("log.txt", true, true, ILM_UNIX);

	KFileSystem::Manager->Init();
	KFileSystem::Manager->AddSystem("../Sponza.zip", -1, FST_ZIP);
	KFileSystem::Manager->AddSystem(".", 0, FST_NATIVE);
	KFileSystem::Manager->AddSystem("../", 1, FST_NATIVE);

	ASSERT_RESULT(InitCodecManager());
	ASSERT_RESULT(InitAssetLoaderManager());

	w.Init();
	w.show();
	int nResult = a.exec();
	w.UnInit();

	ASSERT_RESULT(UnInitCodecManager());
	ASSERT_RESULT(UnInitAssetLoaderManager());

	KLog::Logger->UnInit();
	KFileSystem::Manager->UnInit();

	return nResult;
}
