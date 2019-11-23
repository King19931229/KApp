#pragma once
#include "Internal/ECS/KECS.h"

class KComponentBase
{
protected:
	ComponentType m_Type;
	KEntity* m_EntityHandle;
public:
	KComponentBase(ComponentType type)
		: m_Type(type),
		m_EntityHandle(nullptr)
	{}

	virtual ~KComponentBase() {}

	inline void RegisterEntityHandle(KEntity* entity) { m_EntityHandle = entity; }
	inline void UnRegisterEntityHandle() { m_EntityHandle = nullptr; }

	inline KEntity* GetEntityHandle() { return m_EntityHandle; }
	inline ComponentType GetType() const { return m_Type; }
};