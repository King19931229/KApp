#pragma once
#include "KRender/Interface/IKRenderScene.h"

struct IKScene
{
	virtual ~IKScene() {}

	virtual bool Init(IKRenderScene* renderScene) = 0;
	virtual bool UnInit() = 0;

	virtual bool Add(IKEntityPtr entity) = 0;
	virtual bool Remove(IKEntityPtr entity) = 0;
	virtual bool Move(IKEntityPtr entity) = 0;

	virtual bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result) = 0;
	virtual bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntityPtr& result) = 0;

	virtual bool Save(const char* filename) = 0;
	virtual bool Load(const char* filename) = 0;
};

typedef std::unique_ptr<IKScene> IKScenePtr;