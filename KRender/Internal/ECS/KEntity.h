#pragma once
#include "KECS.h"
#include <unordered_map>
#include "Internal/KConstantDefinition.h"

class KEntity
{
protected:
	typedef std::unordered_map<ComponentType, KComponentBase*> ComponentMap;
	ComponentMap m_Components;



	size_t m_Id;
public:
	KEntity(size_t id);
	~KEntity();

	bool GetComponentBase(ComponentType type, KComponentBase** pptr);
	template<typename T>
	bool GetComponent(ComponentType type, T** pptr)
	{
		return GetComponentBase(type, (KComponentBase**)(pptr));
	}
	bool HasComponent(ComponentType type);
	bool HasComponents(const ComponentTypeList& components);

	bool RegisterComponentBase(ComponentType type, KComponentBase** pptr);
	template<typename T>
	bool RegisterComponent(ComponentType type, T** pptr)
	{
		return RegisterComponentBase(type, (KComponentBase**)(pptr));
	}
	bool RegisterComponent(ComponentType type)
	{
		return RegisterComponentBase(type, nullptr);
	}
	bool UnRegisterComponent(ComponentType type);
	bool UnRegisterAllComponent();

	inline size_t GetID() { return m_Id; }
};