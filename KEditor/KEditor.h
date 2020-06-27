#pragma once

#include <QtWidgets/QMainWindow>
#include <QLayout>
#include <QDockWidget>
#include "ui_KEditor.h"

#include "KEngine/Interface/IKEngine.h"

class KEMainRenderWidget;
class KEGraphWidget;
class KEResourceBrowser;
class KESceneItemWidget;
class KEManipulatorToolBar;
class KEReflectPropertyWidget;

class KEditor : public QMainWindow
{
	Q_OBJECT
public:
	KEditor(QWidget *parent = Q_NULLPTR);
	~KEditor();

	bool Init();
	bool UnInit();
protected:
	KEMainRenderWidget*	m_RenderWidget;
	KEGraphWidget* m_GraphWidget; 

	QDockWidget*  m_ResourceDock;
	KEResourceBrowser* m_ResourceBrowser;

	QDockWidget* m_SceneItemDock;
	KESceneItemWidget* m_SceneItemWidget;

	QDockWidget* m_PropertyDock;
	KEReflectPropertyWidget* m_PropertyWidget;

	KEManipulatorToolBar* m_ManipulatorToolBar;

	IKEnginePtr m_Engine;
	bool m_bInit;
private:
	Ui::KEditorClass ui;
	bool SetupMenu();
	bool OnOpenGraphWidget();
	bool OnLoadScene();
	bool OnSaveScene();
};
