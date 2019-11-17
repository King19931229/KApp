#pragma once
#include "Internal/Entity/KComponent.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

class KTransformComponent : KComponentBase
{
protected:
	glm::vec3 m_Position;
	glm::vec3 m_Scale;
	glm::quat m_Rotate;	
public:
	KTransformComponent()
		: KComponentBase(CT_TRANSFORM),
		m_Position(glm::vec3(0.0f)),
		m_Scale(glm::vec3(1.0f)),
		m_Rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
	{}
	virtual ~KTransformComponent() {}

	glm::quat& GetRotate() { return m_Rotate; }
	glm::vec3& GetScale() { return m_Scale; }
	glm::vec3& GetPosition() { return m_Position; }

	inline glm::mat4 FinalTransform() const
	{
		glm::mat4 rotate = glm::mat4_cast(m_Rotate);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);
		return translate * rotate * scale;
	}
};