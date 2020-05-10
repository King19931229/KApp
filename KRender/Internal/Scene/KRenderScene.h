#pragma once
#include "KBase/Publish/KConfig.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Publish/KCamera.h"
#include "KSceneManagerBase.h"
#include "Interface/IKRenderScene.h"

class KRenderScene : public IKRenderScene
{
protected:
	KSceneManagerBase* m_SceneMgr;
	bool m_EnableDebugRender;
public:
	KRenderScene();
	virtual ~KRenderScene();

	bool Init(SceneManagerType type, float initialSize, const glm::vec3& initialPos) override;
	bool UnInit() override;

	bool Add(IKEntityPtr entity) override;
	bool Remove(IKEntityPtr entity) override;
	bool Move(IKEntityPtr entity) override;

	bool GetRenderComponent(const KCamera& camera, std::vector<KRenderComponent*>& result);
	bool GetRenderComponent(const KAABBBox& bound, std::vector<KRenderComponent*>& result);

	bool GetSceneObjectBound(KAABBBox& box);

	bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result) override;
	bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntityPtr& result) override;

	void EnableDebugRender(bool enable) override { m_EnableDebugRender = enable; }
};