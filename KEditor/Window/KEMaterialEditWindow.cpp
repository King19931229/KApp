#include "KEMaterialEditWindow.h"

KEMaterialEditWindow::KEMaterialEditWindow(QWidget *parent)
	: QMainWindow(parent),
	m_MainWindow(parent),
	m_RenderWidget(nullptr)
{
	Init();
	setAttribute(Qt::WA_DeleteOnClose, true);
}

KEMaterialEditWindow::~KEMaterialEditWindow()
{
	UnInit();
}

QSize KEMaterialEditWindow::sizeHint() const
{
	assert(m_MainWindow);
	int width = m_MainWindow->width() * 2 / 3;
	int height = m_MainWindow->height() * 2 / 3;
	return QSize(width, height);
}

void KEMaterialEditWindow::resizeEvent(QResizeEvent* event)
{
}

bool KEMaterialEditWindow::Init()
{
	UnInit();

	m_RenderWidget = KNEW KEMaterialRenderWidget();
	m_RenderWidget->Init(KEngineGlobal::Engine);
	setCentralWidget(m_RenderWidget);

	IKRenderCore* renderCore = KEngineGlobal::Engine->GetRenderCore();
	IKRenderDispatcher* renderer = renderCore->GetRenderDispatcher();
	renderer->SetCallback(m_RenderWidget->GetRenderWindow(), &m_OnRenderCallBack);

	m_OnRenderCallBack = [this](IKRenderDispatcher* dispatcher, uint32_t chainImageIndex, uint32_t frameIndex)
	{
		// TODO Set MiniScene and MiniCamera
	};

	return true;
}

bool KEMaterialEditWindow::UnInit()
{
	setCentralWidget(nullptr);
	if (m_RenderWidget)
	{
		IKRenderCore* renderCore = KEngineGlobal::Engine->GetRenderCore();
		IKRenderDispatcher* renderer = renderCore->GetRenderDispatcher();
		renderer->RemoveCallback(m_RenderWidget->GetRenderWindow());
		m_RenderWidget->UnInit();
	}
	SAFE_DELETE(m_RenderWidget);
	return true;
}

bool KEMaterialEditWindow::SetEditTarget(const std::string& path)
{
	setWindowTitle(path.c_str());
	return true;
}