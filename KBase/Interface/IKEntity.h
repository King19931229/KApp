#pragma once
#include "KBase/Interface/Component/IKComponentBase.h"
#include "KBase/Interface/Component/IKComponentManager.h"

struct IKEntity
{
	virtual bool GetComponentBase(ComponentType type, IKComponentBase** pptr) = 0;

	template<typename T>
	bool GetComponent(ComponentType type, T** pptr)
	{
		return GetComponentBase(type, (IKComponentBase**)(pptr));
	}

	virtual bool HasComponent(ComponentType type) = 0;
	virtual bool HasComponents(const ComponentTypeList& components) = 0;

	virtual bool RegisterComponentBase(ComponentType type, IKComponentBase** pptr) = 0;

	template<typename T>
	bool RegisterComponent(ComponentType type, T** pptr)
	{
		return RegisterComponentBase(type, (IKComponentBase**)(pptr));
	}

	bool RegisterComponent(ComponentType type)
	{
		return RegisterComponentBase(type, nullptr);
	}

	virtual bool UnRegisterComponent(ComponentType type) = 0;
	virtual bool UnRegisterAllComponent() = 0;

	virtual size_t GetID() const = 0;
};