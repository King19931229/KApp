#include "KEntity.h"
#include "Component/KComponentBase.h"
#include "KECSGlobal.h"

#include <assert.h>

KEntity::KEntity(size_t id)
	: m_Id(id)
{
}

KEntity::~KEntity()
{
	assert(m_Components.empty() && "Components not empty");
}

bool KEntity::GetComponent(ComponentType type, KComponentBase** pptr)
{
	if(pptr)
	{
		auto it = m_Components.find(type);
		if(it != m_Components.end())
		{
			*pptr = it->second;
			return true;
		}
		else
		{
			*pptr = nullptr;
			return false;
		}
	}
	return false;
}

bool KEntity::HasComponent(ComponentType type)
{
	auto it = m_Components.find(type);
	return it != m_Components.end();
}

bool KEntity::HasComponents(const ComponentTypeList& components)
{
	for(const ComponentType& type : components)
	{
		if(!HasComponent(type))
		{
			return false;
		}
	}
	return true;
}

bool KEntity::RegisterComponent(ComponentType type)
{
	auto it = m_Components.find(type);
	if(it == m_Components.end())
	{
		KComponentBase* component = KECSGlobal::ComponentManager.Alloc(type);
		m_Components.insert(ComponentMap::value_type(type, component));
		component->RegisterEntityHandle(this);
		return true;
	}
	else
	{
		return false;
	}
}

bool KEntity::UnRegisterComponent(ComponentType type)
{
	auto it = m_Components.find(type);
	if(it != m_Components.end())
	{
		KComponentBase*& component = it->second;
		component->UnRegisterEntityHandle();
		m_Components.erase(it);
		KECSGlobal::ComponentManager.Free(component);
		return true;
	}
	else
	{
		return false;
	}
}

bool KEntity::UnRegisterAllComponent()
{
	for(auto it = m_Components.begin(), itEnd = m_Components.end(); it != itEnd; ++it)
	{
		KComponentBase*& component = it->second;
		component->UnRegisterEntityHandle();
		KECSGlobal::ComponentManager.Free(component);
	}
	m_Components.clear();
	return true;
}