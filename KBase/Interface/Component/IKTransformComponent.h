#pragma once
#include "IKComponentBase.h"
#include "glm/glm.hpp"

struct IKTransformComponent : public IKComponentBase
{
	IKTransformComponent()
		: IKComponentBase(CT_TRANSFORM)
	{
	}
	virtual ~IKTransformComponent() {}

	virtual const glm::quat& GetRotate() const = 0;
	virtual const glm::vec3& GetScale() const = 0;
	virtual const glm::vec3& GetPosition() const = 0;
	virtual void SetRotate(const glm::quat& rotate) = 0;
	virtual void SetRotate(const glm::mat3& rotate) = 0;
	virtual void SetScale(const glm::vec3& scale) = 0;
	virtual void SetPosition(const glm::vec3& position) = 0;
	virtual const glm::mat4& GetFinal() const = 0;
	virtual void SetFinal(const glm::mat4& final) = 0;
};