#pragma once
#include "KBase/Publish/KConfig.h"
#include "Internal/ECS/KEntity.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Publish/KCamera.h"
#include "KSceneManagerBase.h"
#include "glm/glm.hpp"

class KScene
{
protected:
	KSceneManagerBase* m_SceneMgr;
public:
	KScene();
	~KScene();

	bool Init(SceneManagerType type, float initialSize, const glm::vec3& initialPos);
	bool UnInit();

	bool Add(KEntity* entity);
	bool Remove(KEntity* entity);
	bool Move(KEntity* entity);

	bool GetVisibleComponent(const KCamera& camera, std::vector<KRenderComponent*>& result);
	bool GetDebugComponent(std::vector<KRenderComponent*>& result);

	bool Load(const char* filename);
	bool Save(const char* filename);
};