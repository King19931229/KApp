#pragma once
#include "Interface/IKScene.h"
#include <unordered_set>

class KScene : public IKScene
{
protected:
	IKRenderScene* m_RenderScene;
	EntitySetType m_Entities;

	static const char* msSceneKey;
	static const char* msCameraKey;
	static const char* msEntityKey;
public:
	KScene();
	~KScene();

	virtual bool Init(IKRenderScene* renderScene);
	virtual bool UnInit();

	virtual bool Add(IKEntityPtr entity);
	virtual bool Remove(IKEntityPtr entity);
	virtual bool Move(IKEntityPtr entity);

	virtual const EntitySetType& GetEntities() const;

	virtual bool CreateTerrain(const glm::vec3& center, float size, float height, const KTerrainContext& context);
	virtual bool DestroyTerrain();
	virtual IKTerrainPtr GetTerrain();

	virtual bool Clear();

	virtual bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result);
	virtual bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntityPtr& result);

	virtual bool RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntityPtr>& result);
	virtual bool CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntityPtr& result);

	virtual bool Save(const char* filename);
	virtual bool Load(const char* filename);
};