#include "KEEntitySelector.h"
#include "KEditorGlobal.h"
#include "Widget/KESceneItemWidget.h"

KEEntitySelector::KEEntitySelector()
	: m_SceneItemWidget(nullptr)
{
}

KEEntitySelector::~KEEntitySelector()
{
	assert(m_SceneItemWidget == nullptr);
	assert(m_SelectEntites.empty());
}

bool KEEntitySelector::Init(KESceneItemWidget* sceneItemWidget)
{
	m_SceneItemWidget = sceneItemWidget;
	m_SelectEntites.clear();
	return true;
}

bool KEEntitySelector::UnInit()
{
	m_SceneItemWidget = nullptr;
	m_SelectEntites.clear();
	return true;
}

bool KEEntitySelector::Add(KEEntityPtr entity)
{
	if (entity)
	{
		auto it = m_SelectEntites.find(entity->soul->GetID());
		if (it == m_SelectEntites.end())
		{
			m_SelectEntites.insert(std::make_pair(entity->soul->GetID(), entity));

			if (m_SceneItemWidget)
			{
				m_SceneItemWidget->Select(entity, true);
			}

			KReflectionObjectBase* reflection = nullptr;
			entity->soul->QueryReflection(&reflection);
			KEditorGlobal::ReflectionManager.Add(reflection);

			KEditorGlobal::EntityManipulator.UpdateGizmoTransform();
			return true;
		}
	}
	return false;
}

bool KEEntitySelector::Remove(KEEntityPtr entity)
{
	if (entity)
	{
		return Remove(entity->soul->GetID());
	}
	return false;
}

bool KEEntitySelector::Remove(IKEntity::IDType id)
{
	auto it = m_SelectEntites.find(id);
	if (it != m_SelectEntites.end())
	{
		KEEntityPtr entity = it->second;
		m_SelectEntites.erase(it);

		if (m_SceneItemWidget)
		{
			m_SceneItemWidget->Select(entity, false);
		}

		KReflectionObjectBase* reflection = nullptr;
		entity->soul->QueryReflection(&reflection);
		KEditorGlobal::ReflectionManager.Remove(reflection);

		KEditorGlobal::EntityManipulator.UpdateGizmoTransform();
		return true;
	}
	return false;
}

bool KEEntitySelector::Contain(KEEntityPtr entity)
{
	if (entity)
	{
		return Contain(entity->soul->GetID());
	}
	return false;

}

bool KEEntitySelector::Contain(IKEntity::IDType id)
{
	auto it = m_SelectEntites.find(id);
	if (it != m_SelectEntites.end())
	{
		return true;
	}
	return false;
}

bool KEEntitySelector::Empty()
{
	return m_SelectEntites.empty();
}

bool KEEntitySelector::Clear()
{
	if (m_SceneItemWidget)
	{
		m_SceneItemWidget->ClearSelection();
	}
	m_SelectEntites.clear();
	return true;
}