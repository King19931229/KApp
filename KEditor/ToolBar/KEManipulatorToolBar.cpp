#include "KEManipulatorToolBar.h"
#include "KRender/Publish/KCamera.h"

KEManipulatorToolBar::KEManipulatorToolBar(QWidget *parent)
	: m_MoveGizmoAction(nullptr),
	m_RotateGizmoAction(nullptr),
	m_ScaleGizmoAction(nullptr),
	m_Combo(nullptr),
	m_CameraSpeedSpinBox(nullptr),
	m_CameraSpeedSlider(nullptr),
	m_CameraControl(nullptr)
{
}

KEManipulatorToolBar::~KEManipulatorToolBar()
{
	ASSERT_RESULT(!m_MoveGizmoAction);
	ASSERT_RESULT(!m_RotateGizmoAction);
	ASSERT_RESULT(!m_ScaleGizmoAction);
	ASSERT_RESULT(!m_Combo);
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

bool KEManipulatorToolBar::Init(IKCameraController* cameraControl, IKGizmoPtr gizmo)
{
	UnInit();

	m_CameraControl = cameraControl;
	m_Gizmo = gizmo;

	m_MoveGizmoAction = KNEW QAction(QIcon(":/images/move_gizmo.png"), "MoveGizmo", nullptr);
	m_MoveGizmoAction->setCheckable(true);

	m_RotateGizmoAction = KNEW QAction(QIcon(":/images/rotate_gizmo.png"), "RotateGizmo", nullptr);
	m_RotateGizmoAction->setCheckable(true);

	m_ScaleGizmoAction = KNEW QAction(QIcon(":/images/scale_gizmo.png"), "ScaleGizmo", nullptr);
	m_ScaleGizmoAction->setCheckable(true);

	QObject::connect(m_MoveGizmoAction, &QAction::triggered, [this](bool checked)
	{
		m_Gizmo->SetType(GizmoType::GIZMO_TYPE_MOVE);
		m_MoveGizmoAction->setChecked(true);
		m_RotateGizmoAction->setChecked(false);
		m_ScaleGizmoAction->setChecked(false);
	});
	QObject::connect(m_RotateGizmoAction, &QAction::triggered, [this](bool checked)
	{
		m_Gizmo->SetType(GizmoType::GIZMO_TYPE_ROTATE);
		m_MoveGizmoAction->setChecked(false);
		m_RotateGizmoAction->setChecked(true);
		m_ScaleGizmoAction->setChecked(false);
	});
	QObject::connect(m_ScaleGizmoAction, &QAction::triggered, [this](bool checked)
	{
		m_Gizmo->SetType(GizmoType::GIZMO_TYPE_SCALE);
		m_MoveGizmoAction->setChecked(false);
		m_RotateGizmoAction->setChecked(false);
		m_ScaleGizmoAction->setChecked(true);
	});
	addActions({ m_MoveGizmoAction ,m_RotateGizmoAction, m_ScaleGizmoAction });

	m_Combo = KNEW QComboBox();
	m_Combo->addItem(LOCAL);
	m_Combo->addItem(WORLD);
	m_Combo->setMinimumWidth(70);

	QObject::connect(m_Combo, &QComboBox::currentTextChanged, [this](QString text)
	{
		if (text == LOCAL)
		{
			m_Gizmo->SetManipulateMode(GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL);
		}
		else
		{
			m_Gizmo->SetManipulateMode(GizmoManipulateMode::GIZMO_MANIPULATE_WORLD);
		}
	});
	addWidget(m_Combo);

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
	m_CameraSpeedSlider->setMaximumWidth(100);
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
	SAFE_DELETE(m_MoveGizmoAction);
	SAFE_DELETE(m_RotateGizmoAction);
	SAFE_DELETE(m_ScaleGizmoAction);
	SAFE_DELETE(m_Combo);
	SAFE_DELETE(m_CameraSpeedSlider);
	SAFE_DELETE(m_CameraSpeedSpinBox);
	m_CameraControl = nullptr;
	m_Gizmo = nullptr;
	return true;
}