#pragma once
#include <map>

#include "KECS.h"

class KEntity
{
protected:
	typedef std::map<ComponentType, KComponentBasePtr> ComponentMap;
	ComponentMap m_Components;
	size_t m_Id;
public:
	KEntity(size_t id);
	~KEntity();

	bool GetComponent(ComponentType type, KComponentBasePtr& ptr);
	bool HasComponent(ComponentType type);

	bool HasComponents(const ComponentTypeList& components);

	bool RegisterComponent(KComponentBasePtr component);
	bool UnRegisterComponent(ComponentType type);

	inline size_t GetID() { return m_Id; }
};