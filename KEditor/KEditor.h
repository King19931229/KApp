#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_KEditor.h"

class KEditor : public QMainWindow
{
	Q_OBJECT

public:
	KEditor(QWidget *parent = Q_NULLPTR);

private:
	Ui::KEditorClass ui;
};
