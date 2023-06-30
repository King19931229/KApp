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
	std::string m_Name;
	KSceneManagerBase* m_SceneMgr;
	std::unordered_set<EntityObserverFunc*> m_Observers;
	IKTerrainPtr m_Terrain;

	void OnEntityChange(EntitySceneOp op, IKEntity* entity);
public:
	KRenderScene();
	virtual ~KRenderScene();

	bool Init(const std::string& name, SceneManagerType type, float initialSize, const glm::vec3& initialPos) override;
	bool UnInit() override;

	const std::string& GetName() const override;

	bool Add(IKEntity* entity) override;
	bool Remove(IKEntity* entity) override;
	bool Transform(IKEntity* entity) override;

	bool CreateTerrain(const glm::vec3& center, float size, float height, const KTerrainContext& context) override;
	bool DestroyTerrain() override;
	IKTerrainPtr GetTerrain() override;

	bool RegisterEntityObserver(EntityObserverFunc* func) override;
	bool UnRegisterEntityObserver(EntityObserverFunc* func) override;

	bool GetVisibleEntities(const KCamera& camera, std::vector<IKEntity*>& result) override;
	bool GetVisibleEntities(const KAABBBox& bound, std::vector<IKEntity*>& result) override;

	bool GetDebugEntities(std::vector<IKEntity*>& result) override;

	bool GetSceneObjectBound(KAABBBox& box) override;

	bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntity*>& result) override;
	bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntity*& result) override;

	bool RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntity*>& result) override;
	bool CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntity*& result) override;

	bool GetAllEntities(std::vector<IKEntity*>& result) override;
};