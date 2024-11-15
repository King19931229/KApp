#pragma once
#include "glm/glm.hpp"
#include "KRender/Publish/KCamera.h"
#include <memory>
#include <functional>

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

typedef std::function<void(const glm::mat4& currentTransform)> KGizmoTransformCallback;
typedef std::function<void(bool)> KGizmoTriggerCallback;

struct IKGizmo
{
	virtual ~IKGizmo() {}
	virtual GizmoType GetType() const = 0;
	virtual bool SetType(GizmoType type) = 0;

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

	virtual bool RegisterTransformCallback(KGizmoTransformCallback* callback) = 0;
	virtual bool UnRegisterTransformCallback(KGizmoTransformCallback* callback) = 0;

	virtual bool RegisterTriggerCallback(KGizmoTriggerCallback* callback) = 0;
	virtual bool UnRegisterTriggerCallback(KGizmoTriggerCallback* callback) = 0;

	virtual bool IsTriggered() const = 0;
};

typedef std::shared_ptr<IKGizmo> IKGizmoPtr;
IKGizmoPtr CreateGizmo();

#include "KRender/Interface/IKRenderDevice.h"

struct IKCameraCube
{
	virtual ~IKCameraCube() {}

	virtual bool Init(IKRenderDevice* renderDevice, KCamera* camera) = 0;
	virtual bool UnInit() = 0;

	virtual float GetDisplayScale() const = 0;
	virtual void SetDisplayScale(float scale) = 0;

	virtual void SetScreenSize(unsigned int width, unsigned int height) = 0;

	virtual void Update(float dt) = 0;

	virtual void OnMouseDown(unsigned int x, unsigned int y) = 0;
	virtual void OnMouseMove(unsigned int x, unsigned int y) = 0;
	virtual void OnMouseUp(unsigned int x, unsigned int y) = 0;
};

typedef std::shared_ptr<IKCameraCube> IKCameraCubePtr;
IKCameraCubePtr CreateCameraCube();