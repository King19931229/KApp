#pragma once
#include <QDockWidget>
#include <QMainWindow>
#include "Widget/Material/KEMaterialRenderWidget.h"
#include "Widget/Material/KEMaterialPropertyWidget.h"
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKCameraController.h"
#include "KBase/Interface/IKFileSystem.h"

// 待重构
class KEMaterialEditWindow : public QMainWindow
{
	Q_OBJECT
protected:
	QWidget* m_MainWindow;
	KEMaterialRenderWidget* m_RenderWidget;

	KEMaterialPropertyWidget* m_PropertyWidget;
	QDockWidget* m_PropertyDockWidget;

	IKRenderScenePtr m_MiniScene;
	KCamera m_MiniCamera;
	IKRenderer::OnWindowRenderCallback m_OnRenderCallBack;
	IKEntityPtr m_PreviewEntity;
	IKCameraPreviewControllerPtr m_CameraController;

	IKFileSystem* m_FileSys;
	std::string m_MaterialPath;

	int32_t m_EntityIdx;

	bool InitToolBar();
	bool Init();
	bool UnInit();
	bool RefreshPreview();
	
	bool OnSave();
	bool OnReload();
public:
	KEMaterialEditWindow(QWidget *parent = Q_NULLPTR);
	~KEMaterialEditWindow();

	bool SetEditTarget(IKFileSystem* fileSys, const std::string& path);

	QSize sizeHint() const override;
	void resizeEvent(QResizeEvent* event) override;
};