#pragma once
#include "Interface/Entity/IKEntity.h"
#include <unordered_map>

class KEntity : public IKEntity
{
protected:
	typedef std::unordered_map<ComponentType, IKComponentBase*> ComponentMap;
	ComponentMap m_Components;

	size_t m_Id;
public:
	KEntity(size_t id);
	~KEntity();

	bool GetComponentBase(ComponentType type, IKComponentBase** pptr) override;
	bool HasComponent(ComponentType type) override;
	bool HasComponents(const ComponentTypeList& components) override;

	bool RegisterComponentBase(ComponentType type, IKComponentBase** pptr) override;
	bool UnRegisterComponent(ComponentType type) override;
	bool UnRegisterAllComponent() override;

	bool GetBound(KAABBBox& bound) override;
	bool GetTransform(glm::mat4& transform) override;
	bool SetTransform(const glm::mat4& transform) override;
	bool Intersect(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& result, const float* maxDistance) override;

	bool Save(IKXMLElementPtr element) override;
	bool Load(IKXMLElementPtr element) override;

	size_t GetID() const override { return m_Id; }
};