#include "KGizmo.h"

IKGizmoPtr CreateGizmo(GizmoType type)
{
	switch (type)
	{
	case GizmoType::GIZMO_TYPE_MOVE:
		return IKGizmoPtr(new KMoveGizmo());
	case GizmoType::GIZMO_TYPE_ROTATE:
		return IKGizmoPtr(new KRotateGizmo());
	case GizmoType::GIZMO_TYPE_SCALE:
		return IKGizmoPtr(new KScaleGizmo());
	default:
		return nullptr;
	}
}

IKGizmoGroupPtr CreateGizmoGroup()
{
	return nullptr;
	//return IKGizmoGroupPtr(new KGizmo());
}