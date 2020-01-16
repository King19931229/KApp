#pragma once

#include <QtWidgets/QMainWindow>
#include <QLayout>
#include "KRender/Interface/IKRenderCore.h"
#include "ui_KEditor.h"

class KERenderWidget;

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
	IKRenderWindowPtr m_RenderWindow;
	IKRenderDevicePtr m_RenderDevice;
	IKRenderCorePtr m_RenderCore;
	bool m_bInit;
private:
	Ui::KEditorClass ui;
};
