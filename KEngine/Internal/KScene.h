#pragma once
#include "Interface/IKScene.h"

class KScene : public IKScene
{
public:
	KScene();
	~KScene();

	virtual bool Init(IKRenderScene* renderScene) = 0;
	virtual bool UnInit() = 0;

	virtual bool Save(const char* filename) = 0;
	virtual bool Load(const char* filename) = 0;
};