#pragma once
#include "KBase/Interface/Component/IKComponentBase.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/IKXML.h"
#include "glm/glm.hpp"
#include "KBase/Publish/KAABBBox.h"

struct IKEntity
{
	RTTR_ENABLE()
	RTTR_REGISTRATION_FRIEND
public:
	virtual ~IKEntity() {}

	typedef size_t IDType;
	virtual size_t GetID() const = 0;

	virtual const std::string& GetName() const = 0;
	virtual void SetName(const std::string& name) = 0;

	// Component function
	virtual bool HasComponent(ComponentType type) = 0;
	virtual bool HasComponents(const ComponentTypeList& components) = 0;
	virtual bool GetComponentBase(ComponentType type, IKComponentBase** pptr) = 0;
	virtual bool RegisterComponentBase(ComponentType type, IKComponentBase** pptr) = 0;
	virtual bool UnRegisterComponent(ComponentType type) = 0;
	virtual bool UnRegisterAllComponent() = 0;

	// Utility function
	virtual bool GetLocalBound(KAABBBox& bound) = 0;
	virtual bool GetBound(KAABBBox& bound) = 0;
	virtual bool GetTransform(glm::mat4& transform) = 0;
	virtual bool SetTransform(const glm::mat4& transform) = 0;
	virtual bool Intersect(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& result, const float* maxDistance = nullptr) = 0;

	// Save load function
	virtual bool Save(IKXMLElementPtr element) = 0;
	virtual bool Load(IKXMLElementPtr element) = 0;

	// Tick function
	virtual bool Tick(float dt) = 0;
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

	virtual bool QueryReflection(KReflectionObjectBase** ppObject) = 0;
};

typedef std::shared_ptr<IKEntity> IKEntityPtr;
typedef std::function<void(IKEntityPtr&)> KEntityViewFunc;