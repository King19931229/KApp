#pragma once
#include "glm/glm.hpp"
#include "KRender/Publish/KCamera.h"
#include <memory>

enum class GizmoManipulateMode
{
	GIZMO_MANIPULATE_LOCAL,
	GIZMO_MANIPULATE_WORLD
};

enum class GizmoType
{
	GIZMO_TYPE_MOVE,
	GIZMO_TYPE_ROTATE,
	GIZMO_TYPE_SCALE
};

struct IKGizmo
{
	virtual ~IKGizmo() {}
	virtual GizmoType GetType() const = 0;

	virtual bool Init(const KCamera* camera) = 0;
	virtual bool UnInit() = 0;

	virtual void Enter() = 0;
	virtual void Leave() = 0;

	virtual const glm::mat4& GetMatrix() const = 0;

	virtual GizmoManipulateMode GetManipulateMode() const = 0;
	virtual void SetManipulateMode(GizmoManipulateMode mode) = 0;
};

typedef std::shared_ptr<IKGizmo> IKGizmoPtr;

IKGizmoPtr CreateGizmo(GizmoType type);