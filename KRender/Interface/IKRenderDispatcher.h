#pragma once

#include "KRender/Interface/IKRenderScene.h"

class IKRenderDispatcher
{
public:
	typedef std::function<void(IKRenderDispatcher*, uint32_t, uint32_t)> OnWindowRenderCallback;
	virtual bool SetSceneCamera(IKRenderScene* scene, const KCamera* camera) = 0;
	virtual bool SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback) = 0;
	virtual bool RemoveCallback(IKRenderWindow* window) = 0;
};