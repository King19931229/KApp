#include "KEditor.h"
#include "KEditorGlobal.h"
#include "Widget/KERenderWidget.h"
#include "Widget/KEPostProcessGraphWidget.h"
#include "Widget/KESceneItemWidget.h"
#include "Widget/KEReflectPropertyWidget.h"
#include "Graph/KEGraphRegistrar.h"
#include "Other/KEQtRenderWindow.h"
#include "Browser/KEResourceBrowser.h"
#include "ToolBar/KEManipulatorToolBar.h"
#include <QTextCodec>
#include <QFileDialog>
#include <QMessageBox>
#include <assert.h>

KEditor::KEditor(QWidget *parent)
	: QMainWindow(parent),
	m_RenderWidget(nullptr),
	m_GraphWidget(nullptr),
	m_ResourceDock(nullptr),
	m_ResourceBrowser(nullptr),
	m_SceneItemDock(nullptr),
	m_SceneItemWidget(nullptr),
	m_PropertyDock(nullptr),
	m_PropertyWidget(nullptr),
	m_ManipulatorToolBar(nullptr),
	m_Engine(nullptr),
	m_bInit(false)
{
	ui.setupUi(this);
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
	SetupMenu();
}

KEditor::~KEditor()
{
	assert(m_RenderWidget == nullptr);
	assert(m_GraphWidget == nullptr);
	assert(m_ResourceBrowser == nullptr);
	assert(m_Engine == nullptr);
}

bool KEditor::SetupMenu()
{
	QAction *action = nullptr;
	
	action = ui.menu->addAction(QString::fromLocal8Bit("LoadScene"));
	QObject::connect(action, &QAction::triggered, this, &KEditor::OnLoadScene);

	action = ui.menu->addAction(QString::fromLocal8Bit("SaveScene"));
	QObject::connect(action, &QAction::triggered, this, &KEditor::OnSaveScene);

	action = ui.menu->addAction(QString::fromLocal8Bit("PostProcessGraph"));
	QObject::connect(action, &QAction::triggered, this, &KEditor::OnOpenGraphWidget);
	
	return true;
}

bool KEditor::OnOpenGraphWidget()
{
	m_GraphWidget->show();
	return true;
}

bool KEditor::OnLoadScene()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Load Scene"), ".", tr("Scene Files(*.scene)"));
	if (!path.isEmpty())
	{
		KEditorGlobal::EntityManipulator.Load(path.toStdString().c_str());
		return true;
	}
	else
	{
		//QMessageBox::critical(NULL, tr("Error"), tr("path<") + path + tr(">error"));
		return false;
	}
}

bool KEditor::OnSaveScene()
{
	QString path = QFileDialog::getSaveFileName(this, tr("Save Scene"), ".", tr("Scene Files(*.scene)"));
	if (!path.isEmpty())
	{
		KEditorGlobal::EntityManipulator.Save(path.toStdString().c_str());
		return true;
	}
	else
	{
		//QMessageBox::critical(NULL, tr("Error"), tr("path<") + path + tr(">error"));
		return false;
	}
}

bool KEditor::Init()
{
	if (!m_bInit)
	{
		// 不允许构建操作进入操作栈
		auto commandLockGuard = KEditorGlobal::CommandInvoker.CreateLockGurad();

		m_RenderWidget = KNEW KERenderWidget(this);

		IKRenderWindowPtr window = IKRenderWindowPtr(KNEW KEQtRenderWindow());
		IKRenderWindow* rawWindow = window.get();
		
		KEngineOptions options;
		options.window.hwnd = (void*)m_RenderWidget->winId();
		options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_EDITOR;

		KEngineGlobal::CreateEngine();
		m_Engine = KEngineGlobal::Engine;
		m_Engine->Init(std::move(window), options);

		IKRenderCore* renderCore = m_Engine->GetRenderCore();

		m_RenderWidget->Init(m_Engine);
		setCentralWidget(m_RenderWidget);

		m_GraphWidget = KNEW KEPostProcessGraphWidget(nullptr);
		m_GraphWidget->Init();
		m_GraphWidget->hide();

		m_ResourceBrowser = KNEW KEResourceBrowser(this);
		m_ResourceBrowser->Init();

		m_ResourceDock = KNEW QDockWidget("Resource Browser", this);
		m_ResourceDock->setWidget(m_ResourceBrowser);
		addDockWidget(Qt::BottomDockWidgetArea, m_ResourceDock);

		m_SceneItemWidget = KNEW KESceneItemWidget(this);
		m_SceneItemWidget->Init();

		m_SceneItemDock = KNEW QDockWidget("Scene", this);
		m_SceneItemDock->setWidget(m_SceneItemWidget);
		addDockWidget(Qt::LeftDockWidgetArea, m_SceneItemDock);

		m_PropertyWidget = KNEW KEReflectPropertyWidget(this);
		m_PropertyWidget->Init();

		m_PropertyDock = KNEW QDockWidget("Property", this);
		m_PropertyDock->setWidget(m_PropertyWidget);
		addDockWidget(Qt::RightDockWidgetArea, m_PropertyDock);

		m_ManipulatorToolBar = KNEW KEManipulatorToolBar(this);
		m_ManipulatorToolBar->setWindowTitle("Manipulator Tool Bar");
		m_ManipulatorToolBar->Init(renderCore->GetCameraController(), renderCore->GetGizmo());
		addToolBar(Qt::TopToolBarArea, m_ManipulatorToolBar);

		KEditorGlobal::ReflectionManager.Init(m_PropertyWidget);
		KEditorGlobal::EntityManipulator.Init(renderCore->GetGizmo(), rawWindow, renderCore->GetCamera(), m_Engine->GetScene(), m_SceneItemWidget);
		KEditorGlobal::EntitySelector.Init(m_SceneItemWidget);
		KEditorGlobal::EntityNamePool.Init();

		m_bInit = true;
		return true;
	}

	return false;
}

bool KEditor::UnInit()
{
	if (m_bInit)
	{
		// 清空操作栈 同时不允许析构操作进入操作栈
		KEditorGlobal::CommandInvoker.Clear();
		auto commandLockGuard = KEditorGlobal::CommandInvoker.CreateLockGurad();

		KEditorGlobal::ReflectionManager.UnInit();
		KEditorGlobal::EntitySelector.UnInit();
		KEditorGlobal::EntityManipulator.UnInit();
		KEditorGlobal::EntityNamePool.UnInit();

		m_Engine->UnInit();
		m_Engine = nullptr;
		KEngineGlobal::DestroyEngine();

		if (m_RenderWidget)
		{
			m_RenderWidget->UnInit();
			SAFE_DELETE(m_RenderWidget);
		}

		if (m_GraphWidget)
		{
			m_GraphWidget->UnInit();
			SAFE_DELETE(m_GraphWidget);
		}

		if (m_ResourceBrowser)
		{
			m_ResourceBrowser->UnInit();
			SAFE_DELETE(m_ResourceBrowser);

			m_ResourceDock->setWidget(nullptr);
			removeDockWidget(m_ResourceDock);
			SAFE_DELETE(m_ResourceDock);
		}

		if (m_SceneItemWidget)
		{
			m_SceneItemWidget->UnInit();
			SAFE_DELETE(m_SceneItemWidget);

			m_SceneItemDock->setWidget(nullptr);
			removeDockWidget(m_SceneItemDock);
			SAFE_DELETE(m_SceneItemDock);
		}

		if (m_PropertyWidget)
		{
			m_PropertyWidget->UnInit();
			SAFE_DELETE(m_PropertyWidget);

			m_PropertyDock->setWidget(nullptr);
			removeDockWidget(m_PropertyDock);
			SAFE_DELETE(m_PropertyDock);
		}

		if (m_ManipulatorToolBar)
		{
			m_ManipulatorToolBar->UnInit();
			SAFE_DELETE(m_ManipulatorToolBar);
		}

		m_bInit = false;
	}

	return true;
}