#include "Interface/IKGizmo.h"
#include "KMoveGizmo.h"
#include "KRotateGizmo.h"

IKGizmoPtr CreateGizmo(GizmoType type)
{
	switch (type)
	{
	case GizmoType::GIZMO_TYPE_MOVE:
		return IKGizmoPtr(new KMoveGizmo());
	case GizmoType::GIZMO_TYPE_ROTATE:
		return IKGizmoPtr(new KRotateGizmo());
	/*case GizmoType::GIZMO_TYPE_SCALE:
		return IKGizmoPtr(new KScaleGizmo());*/
	default:
		return nullptr;
	}
}