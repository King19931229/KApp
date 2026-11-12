#include "KEEntitySelector.h"
#include "KEditorGlobal.h"

KEEntitySelector::KEEntitySelector()
{
}

KEEntitySelector::~KEEntitySelector()
{
	assert(m_SelectEntites.empty());
}

bool KEEntitySelector::Init()
{
	m_SelectEntites.clear();
	return true;
}

bool KEEntitySelector::UnInit()
{
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
		m_SelectEntites.erase(it);

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
	m_SelectEntites.clear();
	KEditorGlobal::EntityManipulator.UpdateGizmoTransform();
	return true;
}