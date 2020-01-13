#include "KEditor.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	KEditor w;
	w.show();
	return a.exec();
}
