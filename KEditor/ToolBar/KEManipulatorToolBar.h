#pragma once
#include <QToolBar>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include "KEditorConfig.h"
#include "KRender/Interface/IKCameraController.h"

class KEManipulatorToolBar : public QToolBar
{
	Q_OBJECT
protected:
	QAction* m_MoveGizmoAction;
	QAction* m_RotateGizmoAction;
	QAction* m_ScaleGizmoAction;

	QComboBox* m_Combo;

	QSlider* m_CameraSpeedSlider;
	QSpinBox* m_CameraSpeedSpinBox;

	IKGizmoPtr m_Gizmo;
	IKCameraMoveController* m_CameraControl;

	static constexpr int CAMERA_SPEED_MIN_SPEED = 1;
	static constexpr int CAMERA_SPEED_MAX_SPEED = 100;
	static constexpr int CAMERA_SPEED_DEFAULT_SPEED = 10;

	static const char* LOCAL;
	static const char* WORLD;

	void SetCameraSpeed(int speed);
public:
	KEManipulatorToolBar(QWidget *parent = Q_NULLPTR);
	~KEManipulatorToolBar();

	bool Init(IKCameraMoveController* cameraControl, IKGizmoPtr gizmo);
	bool UnInit();
};