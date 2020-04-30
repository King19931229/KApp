#include "KEditor.h"
#include "KEditorGlobal.h"
#include "Widget/KERenderWidget.h"
#include "Widget/KEPostProcessGraphWidget.h"
#include "Graph/KEGraphRegistrar.h"
#include "Other/KEQtRenderWindow.h"
#include "Browser/KEResourceBrowser.h"
#include <QTextCodec>
#include <assert.h>

KEditor::KEditor(QWidget *parent)
	: QMainWindow(parent),
	m_RenderWidget(nullptr),
	m_GraphWidget(nullptr),
	m_ResourceDock(nullptr),
	m_ResourceBrowser(nullptr),
	m_SceneItemDock(nullptr),
	m_SceneItemWidget(nullptr),
	m_Engine(nullptr),
	m_bInit(false),
	m_GraphAction(nullptr)
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
	m_GraphAction = ui.menu->addAction(QString::fromLocal8Bit("节点图"));
	QObject::connect(m_GraphAction, &QAction::triggered, this, &KEditor::OnOpenGraphWidget);
	return true;
}

bool KEditor::OnOpenGraphWidget()
{
	m_GraphWidget->show();
	return true;
}

bool KEditor::Init()
{
	if (!m_bInit)
	{
		// 不允许构建操作进入操作栈
		auto commandLockGuard = KEditorGlobal::CommandInvoker.CreateLockGurad();

		m_RenderWidget = new KERenderWidget(this);

		IKRenderWindowPtr window = IKRenderWindowPtr(new KEQtRenderWindow());
		IKRenderWindow* rawWindow = window.get();
		
		KEngineOptions options;
		options.window.hwnd = (void*)m_RenderWidget->winId();
		options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_EDITOR;

		KEngineGlobal::CreateEngine();
		m_Engine = KEngineGlobal::Engine;
		m_Engine->Init(std::move(window), options);

		m_RenderWidget->Init(m_Engine);
		setCentralWidget(m_RenderWidget);

		m_GraphWidget = new KEPostProcessGraphWidget(nullptr);
		m_GraphWidget->Init();
		m_GraphWidget->hide();

		m_ResourceBrowser = new KEResourceBrowser(this);
		m_ResourceBrowser->Init();

		m_ResourceDock = new QDockWidget(this);
		m_ResourceDock->setWidget(m_ResourceBrowser);
		addDockWidget(Qt::BottomDockWidgetArea, m_ResourceDock);

		m_SceneItemWidget = new KESceneItemWidget(this);
		m_SceneItemWidget->Init();

		m_SceneItemDock = new QDockWidget(this);
		m_SceneItemDock->setWidget(m_SceneItemWidget);
		addDockWidget(Qt::LeftDockWidgetArea, m_SceneItemDock);

		IKRenderCore* renderCore = m_Engine->GetRenderCore();

		KEditorGlobal::EntityManipulator.Init(renderCore->GetGizmo(), rawWindow, renderCore->GetCamera(), m_Engine->GetScene(), m_SceneItemWidget);

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

		KEditorGlobal::EntityManipulator.UnInit();

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

		m_bInit = false;
	}

	return true;
}