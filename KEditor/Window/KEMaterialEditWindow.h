#pragma once
#include <QDockWidget>
#include <QMainWindow>
#include "Widget/Material/KEMaterialRenderWidget.h"
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKCameraController.h"

class KEMaterialEditWindow : public QMainWindow
{
	Q_OBJECT
protected:
	QWidget* m_MainWindow;
	KEMaterialRenderWidget* m_RenderWidget;
	IKRenderScenePtr m_MiniScene;
	KCamera m_MiniCamera;
	IKRenderDispatcher::OnWindowRenderCallback m_OnRenderCallBack;
	IKEntityPtr m_PreviewEntity;
	IKCameraPreviewControllerPtr m_CameraController;
	std::string m_MaterialPath;
	int32_t m_EntityIdx;

	bool InitToolBar();
	bool Init();
	bool UnInit();
	bool RefreshPreview();
public:
	KEMaterialEditWindow(QWidget *parent = Q_NULLPTR);
	~KEMaterialEditWindow();

	bool SetEditTarget(const std::string& path);

	QSize sizeHint() const override;
	void resizeEvent(QResizeEvent* event) override;
};