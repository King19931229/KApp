#pragma once
#include "Interface/IKScene.h"
#include <unordered_set>

class KScene : public IKScene
{
protected:
	IKRenderScene* m_RenderScene;
	std::unordered_set<IKEntityPtr> m_Entities;

	static const char* msSceneKey;
	static const char* msEntityKey;
public:
	KScene();
	~KScene();

	virtual bool Init(IKRenderScene* renderScene);
	virtual bool UnInit();

	virtual bool Add(IKEntityPtr entity);
	virtual bool Remove(IKEntityPtr entity);
	virtual bool Move(IKEntityPtr entity);

	virtual bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result);
	virtual bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntityPtr& result);

	virtual bool Save(const char* filename);
	virtual bool Load(const char* filename);
};