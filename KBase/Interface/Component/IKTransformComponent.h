#pragma once
#include "IKComponentBase.h"
#include "glm/glm.hpp"

struct IKTransformComponent;

typedef std::function<void(IKTransformComponent* comp, const glm::vec3& pos, const glm::vec3& scale, const glm::quat& rotate)> KTransformChangeCallback;

struct IKTransformComponent : public IKComponentBase
{
	RTTR_ENABLE(IKComponentBase)
	RTTR_REGISTRATION_FRIEND
public:
	IKTransformComponent()
		: IKComponentBase(CT_TRANSFORM)
	{
	}
	virtual ~IKTransformComponent() {}

	virtual bool RegisterTransformChangeCallback(KTransformChangeCallback* callback) = 0;
	virtual bool UnRegisterTransformChangeCallback(KTransformChangeCallback* callback) = 0;

	virtual const glm::vec3& GetScale() const = 0;
	virtual const glm::vec3& GetPosition() const = 0;

	virtual const glm::quat& GetRotateQuat() const = 0;
	virtual glm::vec3 GetRotateEularAngles() const = 0;

	virtual bool IsStatic() const = 0;

	virtual void SetRotateQuat(const glm::quat& rotate) = 0;
	virtual void SetRotateMatrix(const glm::mat3& rotate) = 0;
	virtual void SetRotateEularAngles(glm::vec3 eularAngles) = 0;

	virtual void SetScale(const glm::vec3& scale) = 0;
	virtual void SetPosition(const glm::vec3& position) = 0;
	virtual const glm::mat4& GetFinal() const = 0;
	virtual const glm::mat4& GetPrevFinal() const = 0;
	virtual void SetFinal(const glm::mat4& final) = 0;
	virtual void SetStatic(bool isStatic) = 0;
};