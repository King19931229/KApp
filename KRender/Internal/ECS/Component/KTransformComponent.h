#pragma once
#include "KComponentBase.h"
#include "Internal/KConstantDefinition.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

class KTransformComponent : public KComponentBase
{
protected:
	glm::vec3 m_Position;
	glm::vec3 m_Scale;
	glm::quat m_Rotate;	
	KConstantDefinition::OBJECT m_FinalTransform;
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

	inline KConstantDefinition::OBJECT&	FinalTransform()
	{
		glm::mat4 rotate = glm::mat4_cast(m_Rotate);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);
		m_FinalTransform.MODEL = translate * rotate * scale;
		return m_FinalTransform;
	}
};