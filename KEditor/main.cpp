#include "KEditor.h"

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

	w.Init();
	w.show();
	int nResult = a.exec();
	w.UnInit();

	return nResult;
}
