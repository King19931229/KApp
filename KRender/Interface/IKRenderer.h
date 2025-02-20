#pragma once

#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKStatistics.h"

class IKRenderer
{
public:
	typedef std::function<void(IKRenderer*, uint32_t)> OnWindowRenderCallback;
	virtual IKRenderScene* GetScene() = 0;
	virtual const KCamera* GetCamera() = 0;
	virtual bool SetCameraCubeDisplay(bool display) = 0;
	virtual bool SetSceneCamera(IKRenderScene* scene, const KCamera* camera) = 0;
	virtual bool SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback) = 0;
	virtual bool RemoveCallback(IKRenderWindow* window) = 0;
};