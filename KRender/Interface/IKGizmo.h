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

	virtual void Update() = 0;

	virtual const glm::mat4& GetMatrix() const = 0;
	virtual void SetMatrix(const glm::mat4& matrix) = 0;

	virtual GizmoManipulateMode GetManipulateMode() const = 0;
	virtual void SetManipulateMode(GizmoManipulateMode mode) = 0;

	virtual float GetDisplayScale() const = 0;
	virtual void SetDisplayScale(float scale) = 0;

	virtual void SetScreenSize(unsigned int width, unsigned int height) = 0;

	virtual void OnMouseDown(unsigned int x, unsigned int y) = 0;
	virtual void OnMouseMove(unsigned int x, unsigned int y) = 0;
	virtual void OnMouseUp(unsigned int x, unsigned int y) = 0;
};

struct IKGizmoGroup : public IKGizmo
{
	virtual GizmoType SetType(GizmoType type) = 0;
};

typedef std::shared_ptr<IKGizmo> IKGizmoPtr;
IKGizmoPtr CreateGizmo(GizmoType type);

typedef std::shared_ptr<IKGizmoGroup> IKGizmoGroupPtr;
IKGizmoGroupPtr CreateGizmoGroup();