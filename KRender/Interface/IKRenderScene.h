#pragma once
#include "KBase/Interface/IKEntity.h"
#include "KRender/Publish/KCamera.h"
#include "glm/glm.hpp"

enum SceneManagerType
{
	SCENE_MANGER_TYPE_OCTREE
};

struct IKRenderScene
{
	virtual ~IKRenderScene() {}

	virtual bool Init(SceneManagerType type, float initialSize, const glm::vec3& initialPos) = 0;
	virtual bool UnInit() = 0;

	virtual bool Add(IKEntityPtr entity) = 0;
	virtual bool Remove(IKEntityPtr entity) = 0;
	virtual bool Move(IKEntityPtr entity) = 0;

	virtual bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result) = 0;
	virtual bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntityPtr& result) = 0;

	virtual void EnableDebugRender(bool enable) = 0;
};