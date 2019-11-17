#include "KEntity.h"
#include "KComponent.h"
#include <assert.h>

KEntity::KEntity(size_t id)
	: m_Id(id)
{
}

KEntity::~KEntity()
{
	assert(m_Components.empty() && "Components not empty");
}

bool KEntity::GetComponent(ComponentType type, KComponentBasePtr& ptr)
{
	auto it = m_Components.find(type);
	if(it != m_Components.end())
	{
		ptr = it->second;
		return true;
	}
	else
	{
		ptr = nullptr;
		return false;
	}
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

bool KEntity::RegisterComponent(KComponentBasePtr component)
{
	if(component != nullptr)
	{
		ComponentType type = component->GetType();
		auto it = m_Components.find(type);
		if(it == m_Components.end())
		{
			m_Components.insert(ComponentMap::value_type(type, component));
			component->RegisterEntityHandle(this);
			return true;
		}
		else
		{
			return false;
		}
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
		KComponentBasePtr& component = it->second;
		component->UnRegisterEntityHandle();
		m_Components.erase(it);
		return true;
	}
	else
	{
		return false;
	}
}