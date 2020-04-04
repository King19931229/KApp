#include "KEditor.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/Component/IKComponentManager.h"

#include <QtWidgets/QApplication>
#include <QFile>
#include <QTextStream>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	KEditor w;

	// https://github.com/Lumyo/darkorange-pyside-stylesheet
	// http://discourse.techart.online/t/release-qt-dark-orange-stylesheet/2287
	QFile file(":/darkorange.qss");
	file.open(QFile::ReadOnly);
	QTextStream filetext(&file);
	QString stylesheet = filetext.readAll();
	a.setStyleSheet(stylesheet);
	a.setStyle("plastique");
	file.close();

	KLog::CreateLogger();
	KLog::Logger->Init("log.txt", true, true, ILM_UNIX);

	KComponent::CreateManager();

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

	KComponent::DestroyManager();

	KFileSystem::Manager->UnInit();
	KFileSystem::DestroyFileManager();

	return nResult;
}
