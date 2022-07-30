#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Publish/KCamera.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "KSceneManagerBase.h"

class KRenderScene : public IKRenderScene
{
protected:
	KSceneManagerBase* m_SceneMgr;
	std::unordered_set<EntityObserverFunc*> m_Observers;
	IKTerrainPtr m_Terrain;

	void OnEntityChange(EntitySceneOp op, IKEntityPtr entity);
public:
	KRenderScene();
	virtual ~KRenderScene();

	bool Init(SceneManagerType type, float initialSize, const glm::vec3& initialPos) override;
	bool UnInit() override;

	bool Add(IKEntityPtr entity) override;
	bool Remove(IKEntityPtr entity) override;
	bool Move(IKEntityPtr entity) override;

	IKTerrainPtr GetTerrain() override;

	bool RegisterEntityObserver(EntityObserverFunc* func) override;
	bool UnRegisterEntityObserver(EntityObserverFunc* func) override;

	bool GetRenderComponent(const KCamera& camera, bool withDebug, std::vector<KRenderComponent*>& result);
	bool GetRenderComponent(const KAABBBox& bound, bool withDebug, std::vector<KRenderComponent*>& result);

	bool GetSceneObjectBound(KAABBBox& box);

	bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result) override;
	bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntityPtr& result) override;

	bool RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntityPtr>& result) override;
	bool CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntityPtr& result) override;

	bool GetAllEntities(std::vector<IKEntityPtr>& result) override;
};