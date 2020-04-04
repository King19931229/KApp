#pragma once
#include "KRender/Interface/IKRenderScene.h"

struct IKScene
{
	virtual ~IKScene() {}

	virtual bool Init(IKRenderScene* renderScene) = 0;
	virtual bool UnInit() = 0;

	virtual bool Save(const char* filename) = 0;
	virtual bool Load(const char* filename) = 0;
};