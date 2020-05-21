#pragma once
#include "IKGizmo.h"
#include "KRender/Publish/KCamera.h"

struct IKCameraController
{
	virtual bool Init(KCamera* camera, IKRenderWindow* window, IKGizmoPtr gizmo) = 0;
	virtual bool UnInit() = 0;
	virtual void SetEnable(bool enable) = 0;
	virtual void SetSpeed(float speed) = 0;
};