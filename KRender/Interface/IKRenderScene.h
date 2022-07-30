#pragma once
#include "KBase/Interface/Entity/IKEntity.h"
#include "KRender/Interface/IKTerrain.h"
#include "KRender/Publish/KCamera.h"
#include "glm/glm.hpp"

enum SceneManagerType
{
	SCENE_MANGER_TYPE_OCTREE
};

enum EntitySceneOp
{
	ESO_ADD,
	ESO_REMOVE,
	ESO_MOVE
};

typedef std::function<void(EntitySceneOp, IKEntityPtr)> EntityObserverFunc;

struct IKRenderScene
{
	virtual ~IKRenderScene() {}

	virtual bool Init(SceneManagerType type, float initialSize, const glm::vec3& initialPos) = 0;
	virtual bool UnInit() = 0;

	virtual bool Add(IKEntityPtr entity) = 0;
	virtual bool Remove(IKEntityPtr entity) = 0;
	virtual bool Move(IKEntityPtr entity) = 0;

	virtual IKTerrainPtr GetTerrain() = 0;

	virtual bool RegisterEntityObserver(EntityObserverFunc* func) = 0;
	virtual bool UnRegisterEntityObserver(EntityObserverFunc* func) = 0;

	virtual bool GetSceneObjectBound(KAABBBox& box) = 0;

	virtual bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result) = 0;
	virtual bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntityPtr& result) = 0;

	virtual bool RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntityPtr>& result) = 0;
	virtual bool CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntityPtr& result) = 0;

	virtual bool GetAllEntities(std::vector<IKEntityPtr>& result) = 0;
};

typedef std::unique_ptr<IKRenderScene> IKRenderScenePtr;

EXPORT_DLL IKRenderScenePtr CreateRenderScene();