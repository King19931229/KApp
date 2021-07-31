#pragma once

#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKStatistics.h"

class IKRenderDispatcher
{
public:
	typedef std::function<void(IKRenderDispatcher*, uint32_t, uint32_t)> OnWindowRenderCallback;
	virtual IKRenderScene* GetScene() = 0;
	virtual const KCamera* GetCamera() = 0;
	virtual bool SetCameraCubeDisplay(bool display) = 0;
	virtual bool SetSceneCamera(IKRenderScene* scene, const KCamera* camera) = 0;
	virtual bool SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback) = 0;
	virtual bool RemoveCallback(IKRenderWindow* window) = 0;
};