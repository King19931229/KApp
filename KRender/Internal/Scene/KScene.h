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
	bool m_EnableDebugRender;
public:
	KScene();
	~KScene();

	bool Init(SceneManagerType type, float initialSize, const glm::vec3& initialPos);
	bool UnInit();

	bool Add(KEntityPtr entity);
	bool Remove(KEntityPtr entity);
	bool Move(KEntityPtr entity);

	bool GetRenderComponent(const KCamera& camera, std::vector<KRenderComponent*>& result);

	inline void EnableDebugRender(bool enable) { m_EnableDebugRender = enable; }

	bool Load(const char* filename);
	bool Save(const char* filename);
};