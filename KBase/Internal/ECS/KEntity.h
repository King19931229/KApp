#pragma once
#include "Interface/Entity/IKEntity.h"
#include <unordered_map>

class KEntity : public IKEntity, public KReflectionObjectBase
{
	RTTR_ENABLE(IKEntity, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	IKComponentBase* m_Components[CT_COUNT];
	size_t m_Id;
	std::string m_Name;

	static const char* msName;
	static const char* msComponent;
	static const char* msComponentType;

	IKComponentBase* SafeGetComponent(ComponentType type)
	{
		IKComponentBase* pRet = nullptr;
		if (GetComponentBase(type, &pRet))
		{
			return pRet;
		}
		return nullptr;
	}

	IKComponentBase* GetRenderComponent() { return SafeGetComponent(CT_RENDER); }
	IKComponentBase* GetDebugComponent() { return SafeGetComponent(CT_DEBUG); }
	IKComponentBase* GetTransformComponent() { return SafeGetComponent(CT_TRANSFORM); }
	IKComponentBase* GetUserComponent() { return SafeGetComponent(CT_USER); }

	KEntity();
public:
	KEntity(size_t id);
	~KEntity();

	bool GetComponentBase(ComponentType type, IKComponentBase** pptr) override;
	bool HasComponent(ComponentType type) override;
	bool HasComponents(const ComponentTypeList& components) override;

	bool RegisterComponentBase(ComponentType type, IKComponentBase** pptr) override;
	bool UnRegisterComponent(ComponentType type) override;
	bool UnRegisterAllComponent() override;

	bool GetLocalBound(KAABBBox& bound) override;
	bool GetBound(KAABBBox& bound) override;
	bool GetTransform(glm::mat4& transform) override;
	bool SetTransform(const glm::mat4& transform) override;
	bool Intersect(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& result, const float* maxDistance) override;

	bool Save(IKXMLElementPtr element) override;
	bool Load(IKXMLElementPtr element) override;

	bool PreTick() override;
	bool PostTick() override;

	size_t GetID() const override { return m_Id; }

	const std::string& GetName() const override  { return m_Name;}
	void SetName(const std::string& name) override { m_Name = name; }

	bool QueryReflection(KReflectionObjectBase** ppObject) override
	{
		if (ppObject)
		{
			*ppObject = static_cast<KReflectionObjectBase*>(this);
			return true;
		}
		return false;
	}
};