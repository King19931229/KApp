#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/Scene/KScene.h"
#include "Publish/KCamera.h"

class KRenderDispatcher
{
public:
	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Visit(KScene* scene, KCamera* camera);
	bool Execute();
};