#pragma once
#include "KBase/Interface/IKXML.h"
#include "KBase/Interface/IKReflection.h"
#include <vector>

enum ComponentType
{
	CT_TRANSFORM,
	CT_RENDER,
	CT_DEBUG,

	CT_USER,

	CT_COUNT,
	CT_UNKNOWN = CT_COUNT
};

typedef std::vector<ComponentType> ComponentTypeList;

struct IKEntity;

class IKComponentBase
{
	RTTR_ENABLE()
	RTTR_REGISTRATION_FRIEND
protected:
	ComponentType m_Type;
	IKEntity* m_EntityHandle;
public:
	IKComponentBase(ComponentType type)
		: m_Type(type),
		m_EntityHandle(nullptr)
	{}

	virtual ~IKComponentBase() {}

	virtual bool Save(IKXMLElementPtr element) = 0;
	virtual bool Load(IKXMLElementPtr element) = 0;

	virtual bool Tick(float dt) { return true; }

	inline void RegisterEntityHandle(IKEntity* entity) { m_EntityHandle = entity; }
	inline void UnRegisterEntityHandle() { m_EntityHandle = nullptr; }

	inline IKEntity* GetEntityHandle() { return m_EntityHandle; }
	inline ComponentType GetType() const { return m_Type; }
};