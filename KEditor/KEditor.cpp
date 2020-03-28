#include "KEditor.h"
#include "KEditorGlobal.h"
#include "Widget/KERenderWidget.h"
#include "Widget/KEPostProcessGraphWidget.h"
#include "Graph/KEGraphRegistrar.h"
#include "Other/KEQtRenderWindow.h"
#include <QTextCodec>
#include <assert.h>

KEditor::KEditor(QWidget *parent)
	: QMainWindow(parent),
	m_RenderWidget(nullptr),
	m_GraphWidget(nullptr),
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
	assert(m_RenderCore == nullptr);
}

bool KEditor::SetupMenu()
{
	m_GraphAction = ui.menu->addAction(QString::fromLocal8Bit("�ڵ�ͼ"));
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
		// �������������������ջ
		auto commandLockGuard = KEditorGlobal::CommandInvoker.CreateLockGurad();

		m_RenderWidget = new KERenderWidget();

		m_RenderCore = CreateRenderCore();

		m_RenderWindow = IKRenderWindowPtr((IKRenderWindow*)new KEQtRenderWindow());
		m_RenderWindow->Init((void*)m_RenderWidget->winId());

		m_RenderDevice = CreateRenderDevice(RENDER_DEVICE_VULKAN);

		m_RenderDevice->Init(m_RenderWindow.get());
		m_RenderCore->Init(m_RenderDevice, m_RenderWindow);

		m_RenderWidget->Init(m_RenderCore);
		setCentralWidget(m_RenderWidget);

		m_GraphWidget = new KEPostProcessGraphWidget();
		m_GraphWidget->Init();
		m_GraphWidget->hide();

		m_bInit = true;
		return true;
	}

	return false;
}

bool KEditor::UnInit()
{
	if (m_bInit)
	{
		// ��ղ���ջ ͬʱ���������������������ջ
		KEditorGlobal::CommandInvoker.Clear();
		auto commandLockGuard = KEditorGlobal::CommandInvoker.CreateLockGurad();

		m_RenderCore->UnInit();
		m_RenderCore = nullptr;

		m_RenderDevice->UnInit();
		m_RenderDevice = nullptr;

		m_RenderWindow->UnInit();
		m_RenderWindow = nullptr;

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

		m_bInit = false;
	}

	return true;
}