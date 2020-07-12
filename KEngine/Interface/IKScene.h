#pragma once
#include "KRender/Interface/IKRenderScene.h"
#include <unordered_set>

struct IKScene
{
	virtual ~IKScene() {}

	virtual bool Init(IKRenderScene* renderScene) = 0;
	virtual bool UnInit() = 0;

	virtual bool Add(IKEntityPtr entity) = 0;
	virtual bool Remove(IKEntityPtr entity) = 0;
	virtual bool Move(IKEntityPtr entity) = 0;

	typedef std::unordered_set<IKEntityPtr> EntitySetType;
	virtual const EntitySetType& GetEntities() const = 0;

	virtual bool Clear() = 0;

	virtual bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result) = 0;
	virtual bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntityPtr& result) = 0;

	virtual bool RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntityPtr>& result) = 0;
	virtual bool CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntityPtr& result) = 0;

	virtual bool Save(const char* filename) = 0;
	virtual bool Load(const char* filename) = 0;
};

typedef std::unique_ptr<IKScene> IKScenePtr;