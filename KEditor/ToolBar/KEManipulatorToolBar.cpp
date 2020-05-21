#include "KEManipulatorToolBar.h"
#include "KRender/Publish/KCamera.h"

KEManipulatorToolBar::KEManipulatorToolBar(QWidget *parent)
	: m_CameraSpeedSpinBox(nullptr),
	m_CameraSpeedSlider(nullptr),
	m_CameraControl(nullptr)
{
}

KEManipulatorToolBar::~KEManipulatorToolBar()
{
	ASSERT_RESULT(!m_CameraSpeedSpinBox);
	ASSERT_RESULT(!m_CameraSpeedSlider);
	ASSERT_RESULT(!m_CameraControl);
}

void KEManipulatorToolBar::SetCameraSpeed(int speed)
{
	float moveSpeed = (float)speed / CAMERA_SPEED_DEFAULT_SPEED;
	if (m_CameraControl)
	{
		m_CameraControl->SetSpeed(speed);
	}
}

bool KEManipulatorToolBar::Init(IKCameraController* cameraControl)
{
	UnInit();

	m_CameraControl = cameraControl;

	m_CameraSpeedSpinBox = KNEW QSpinBox();
	addWidget(m_CameraSpeedSpinBox);

	m_CameraSpeedSlider = KNEW QSlider();
	m_CameraSpeedSlider->setOrientation(Qt::Horizontal);
	m_CameraSpeedSlider->setSingleStep(1);
	addWidget(m_CameraSpeedSlider);

	QObject::connect(m_CameraSpeedSpinBox, SIGNAL(valueChanged(int)),
		m_CameraSpeedSlider, SLOT(setValue(int)));
	QObject::connect(m_CameraSpeedSlider, SIGNAL(valueChanged(int)),
		m_CameraSpeedSpinBox, SLOT(setValue(int)));

	m_CameraSpeedSlider->setMinimum(CAMERA_SPEED_MIN_SPEED);
	m_CameraSpeedSlider->setMaximum(CAMERA_SPEED_MAX_SPEED);
	m_CameraSpeedSpinBox->setMinimum(CAMERA_SPEED_MIN_SPEED);
	m_CameraSpeedSpinBox->setMaximum(CAMERA_SPEED_MAX_SPEED);

	QObject::connect(m_CameraSpeedSlider, &QSlider::valueChanged, [this](int newValue)
	{
		SetCameraSpeed(newValue);
	});
	/*
	QObject::connect(m_CameraSpeedSpinBox, static_cast<void(*)(int)>(QSpinBox::valueChanged), [this](int newValue)
	{
		SetCameraSpeed(newValue);
	});
	*/

	m_CameraSpeedSlider->setValue(CAMERA_SPEED_DEFAULT_SPEED);

	return true;
}

bool KEManipulatorToolBar::UnInit()
{
	SAFE_DELETE(m_CameraSpeedSlider);
	SAFE_DELETE(m_CameraSpeedSpinBox);
	m_CameraControl = nullptr;

	return true;
}