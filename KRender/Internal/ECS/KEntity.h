#pragma once
#include <unordered_map>

#include "KECS.h"

class KEntity
{
protected:
	typedef std::unordered_map<ComponentType, KComponentBase*> ComponentMap;
	ComponentMap m_Components;
	size_t m_Id;
public:
	KEntity(size_t id);
	~KEntity();

	bool GetComponent(ComponentType type, KComponentBase** pptr);
	bool HasComponent(ComponentType type);

	bool HasComponents(const ComponentTypeList& components);

	bool RegisterComponent(ComponentType type, KComponentBase** pptr = nullptr);
	bool UnRegisterComponent(ComponentType type);
	bool UnRegisterAllComponent();

	inline size_t GetID() { return m_Id; }
};