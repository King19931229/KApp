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

	void UpdateTransform()
	{
		glm::mat4 rotate = glm::mat4_cast(m_Rotate);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);
		m_FinalTransform.MODEL = translate * rotate * scale;
	}
public:
	KTransformComponent()
		: KComponentBase(CT_TRANSFORM),
		m_Position(glm::vec3(0.0f)),
		m_Scale(glm::vec3(1.0f)),
		m_Rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
	{}
	virtual ~KTransformComponent() {}

	inline const glm::quat& GetRotate() const { return m_Rotate; }
	inline const glm::vec3& GetScale() const { return m_Scale; }
	inline const glm::vec3& GetPosition() const { return m_Position; }

	void SetRotate(const glm::quat& rotate)
	{
		m_Rotate = rotate;
		UpdateTransform();
	}

	void SetScale(const glm::vec3& scale)
	{
		m_Scale = scale;
		UpdateTransform();
	}

	void SetPosition(const glm::vec3& position)
	{
		m_Position = position;
		UpdateTransform();
	}

	inline const KConstantDefinition::OBJECT& FinalTransform() const
	{		
		return m_FinalTransform;
	}
};