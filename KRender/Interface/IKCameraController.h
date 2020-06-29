#pragma once
#include "IKGizmo.h"
#include "KRender/Publish/KCamera.h"

struct IKCameraMoveController
{
	virtual bool Init(KCamera* camera, IKRenderWindow* window, IKGizmoPtr gizmo) = 0;
	virtual bool UnInit() = 0;
	virtual void SetEnable(bool enable) = 0;
	virtual void SetSpeed(float speed) = 0;
};

struct IKCameraPreviewController
{
	virtual bool Init(KCamera* camera, IKRenderWindow* window) = 0;
	virtual bool UnInit() = 0;
	virtual void SetEnable(bool enable) = 0;
	virtual void SetPreviewCenter(const glm::vec3& center) = 0;
	virtual void Update() = 0;
};

typedef std::unique_ptr<IKCameraPreviewController> IKCameraPreviewControllerPtr;
EXPORT_DLL IKCameraPreviewControllerPtr CreateCameraPreviewController();