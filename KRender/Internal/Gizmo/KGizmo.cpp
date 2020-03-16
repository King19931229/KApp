#include "KGizmo.h"

IKGizmoPtr CreateGizmo()
{
	return IKGizmoPtr(new KGizmo());
}

KGizmo::KGizmo()
	: m_TranslateGizmo(nullptr),
	m_RotateGizmo(nullptr),
	m_ScaleGizmo(nullptr),
	m_CurrentGizmo(nullptr),
	m_Enter(false)
{
	m_TranslateGizmo = IKGizmoPtr(new KMoveGizmo());
	m_RotateGizmo = IKGizmoPtr(new KRotateGizmo());
	m_ScaleGizmo = IKGizmoPtr(new KScaleGizmo());
	m_CurrentGizmo = m_TranslateGizmo;
}

KGizmo::~KGizmo()
{
}

GizmoType KGizmo::GetType() const
{
	return m_CurrentGizmo->GetType();
}

bool KGizmo::SetType(GizmoType type)
{
	glm::mat4 matrix = m_CurrentGizmo->GetMatrix();
	bool enter = m_Enter;

	if (enter)
	{
		Leave();
	}

	switch (type)
	{
	case GizmoType::GIZMO_TYPE_MOVE:
		m_CurrentGizmo = m_TranslateGizmo;
		break;
	case GizmoType::GIZMO_TYPE_ROTATE:
		m_CurrentGizmo = m_RotateGizmo;
		break;
	case GizmoType::GIZMO_TYPE_SCALE:
		m_CurrentGizmo = m_ScaleGizmo;
		break;
	}

	SetMatrix(matrix);

	if (enter)
	{
		Enter();
	}

	return true;
}

bool KGizmo::Init(const KCamera* camera)
{
	if (m_TranslateGizmo->Init(camera) &&
		m_RotateGizmo->Init(camera) &&
		m_ScaleGizmo->Init(camera))
	{
		return true;
	}

	return false;
}

bool KGizmo::UnInit()
{
	if (m_TranslateGizmo->UnInit() &&
		m_RotateGizmo->UnInit() &&
		m_ScaleGizmo->UnInit())
	{
		return true;
	}

	return false;

}

void KGizmo::Enter()
{
	m_CurrentGizmo->Enter();
	m_Enter = true;
}

void KGizmo::Leave()
{
	m_CurrentGizmo->Leave();
	m_Enter = false;
}

void KGizmo::Update()
{
	m_CurrentGizmo->Update();
}

const glm::mat4& KGizmo::GetMatrix() const
{
	return m_CurrentGizmo->GetMatrix();
}

void KGizmo::SetMatrix(const glm::mat4& matrix)
{
	m_CurrentGizmo->SetMatrix(matrix);
}

GizmoManipulateMode KGizmo::GetManipulateMode() const
{
	return m_CurrentGizmo->GetManipulateMode();
}

void KGizmo::SetManipulateMode(GizmoManipulateMode mode)
{
	m_TranslateGizmo->SetManipulateMode(mode);
	m_RotateGizmo->SetManipulateMode(mode);
	m_ScaleGizmo->SetManipulateMode(mode);
}

float KGizmo::GetDisplayScale() const
{
	return m_CurrentGizmo->GetDisplayScale();
}

void KGizmo::SetDisplayScale(float scale)
{
	m_TranslateGizmo->SetDisplayScale(scale);
	m_RotateGizmo->SetDisplayScale(scale);
	m_ScaleGizmo->SetDisplayScale(scale);
}

void KGizmo::SetScreenSize(unsigned int width, unsigned int height)
{
	m_TranslateGizmo->SetScreenSize(width, height);
	m_RotateGizmo->SetScreenSize(width, height);
	m_ScaleGizmo->SetScreenSize(width, height);
}

void KGizmo::OnMouseDown(unsigned int x, unsigned int y)
{
	m_CurrentGizmo->OnMouseDown(x, y);
}

void KGizmo::OnMouseMove(unsigned int x, unsigned int y)
{
	m_CurrentGizmo->OnMouseMove(x, y);
}

void KGizmo::OnMouseUp(unsigned int x, unsigned int y)
{
	m_CurrentGizmo->OnMouseUp(x, y);
}