#pragma once
#include <QToolBar>
#include <QSlider>
#include <QSpinBox>
#include "KEditorConfig.h"
#include "KRender/Interface/IKCameraController.h"

class KEManipulatorToolBar : public QToolBar
{
protected:
	QSlider* m_CameraSpeedSlider;
	QSpinBox* m_CameraSpeedSpinBox;
	IKCameraController* m_CameraControl;

	static constexpr int CAMERA_SPEED_MIN_SPEED = 1;
	static constexpr int CAMERA_SPEED_MAX_SPEED = 100;
	static constexpr int CAMERA_SPEED_DEFAULT_SPEED = 10;

	void SetCameraSpeed(int speed);
public:
	KEManipulatorToolBar(QWidget *parent = Q_NULLPTR);
	~KEManipulatorToolBar();

	bool Init(IKCameraController* cameraControl);
	bool UnInit();
};