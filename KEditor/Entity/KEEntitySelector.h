#pragma once
#include "KEditorConfig.h"
#include <unordered_map>

class KESceneItemWidget;
class KEEntitySelector
{
public:
	typedef std::unordered_map<IKEntity::IDType, KEEntityPtr> EntityCollectionType;
protected:
	EntityCollectionType m_SelectEntites;
	KESceneItemWidget* m_SceneItemWidget;
public:
	KEEntitySelector();
	~KEEntitySelector();

	bool Init(KESceneItemWidget* sceneItemWidget);
	bool UnInit();

	bool Add(KEEntityPtr entity);
	bool Remove(KEEntityPtr entity);
	bool Remove(IKEntity::IDType id);
	bool Contain(KEEntityPtr entity);
	bool Contain(IKEntity::IDType id);
	bool Empty();
	bool Clear();

	inline const EntityCollectionType& GetSelection() const { return m_SelectEntites; }
};