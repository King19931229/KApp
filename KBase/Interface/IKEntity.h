#pragma once
#include "KBase/Interface/Component/IKComponentBase.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "glm/glm.hpp"
#include "KBase/Publish/KAABBBox.h"

struct IKEntity
{
	virtual ~IKEntity() {}

	virtual size_t GetID() const = 0;

	// Component function
	virtual bool HasComponent(ComponentType type) = 0;
	virtual bool HasComponents(const ComponentTypeList& components) = 0;
	virtual bool GetComponentBase(ComponentType type, IKComponentBase** pptr) = 0;
	virtual bool RegisterComponentBase(ComponentType type, IKComponentBase** pptr) = 0;
	virtual bool UnRegisterComponent(ComponentType type) = 0;
	virtual bool UnRegisterAllComponent() = 0;

	// Utility function
	virtual bool GetBound(KAABBBox& bound) = 0;
	virtual bool GetTransform(glm::mat4& transform) = 0;
	virtual bool Intersect(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& result, const float* maxDistance = nullptr) = 0;

	// Template function
	template<typename T>
	bool GetComponent(ComponentType type, T** pptr)
	{
		return GetComponentBase(type, (IKComponentBase**)(pptr));
	}

	template<typename T>
	bool RegisterComponent(ComponentType type, T** pptr)
	{
		return RegisterComponentBase(type, (IKComponentBase**)(pptr));
	}

	bool RegisterComponent(ComponentType type)
	{
		return RegisterComponentBase(type, nullptr);
	}
};

typedef std::shared_ptr<IKEntity> IKEntityPtr;
typedef std::function<void(IKEntityPtr&)> KEntityViewFunc;