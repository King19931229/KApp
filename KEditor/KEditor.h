#pragma once

#include <QtWidgets/QMainWindow>
#include <QLayout>
#include "ui_KEditor.h"

#include "KEngine/Interface/IKEngine.h"

class KERenderWidget;
class KEGraphWidget;

class KEditor : public QMainWindow
{
	Q_OBJECT
public:
	KEditor(QWidget *parent = Q_NULLPTR);
	~KEditor();

	bool Init();
	bool UnInit();
protected:
	KERenderWidget*	m_RenderWidget;
	KEGraphWidget* m_GraphWidget; 
	IKEnginePtr m_Engine;
	bool m_bInit;
private:
	Ui::KEditorClass ui;
	QAction* m_GraphAction;

	bool SetupMenu();

	bool OnOpenGraphWidget();
};
