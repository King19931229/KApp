#pragma once
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "Internal/KConstantDefinition.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "KBase/Publish/KMath.h"

class KTransformComponent : public IKTransformComponent
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
		: m_Position(glm::vec3(0.0f)),
		m_Scale(glm::vec3(1.0f)),
		m_Rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
	{}
	virtual ~KTransformComponent() {}

	const glm::quat& GetRotate() const override { return m_Rotate; }
	const glm::vec3& GetScale() const override { return m_Scale; }
	const glm::vec3& GetPosition() const override { return m_Position; }

	void SetRotate(const glm::quat& rotate) override
	{
		m_Rotate = rotate;
		UpdateTransform();
	}

	void SetRotate(const glm::mat3& rotate) override
	{
		m_Rotate = glm::quat_cast(rotate);
		UpdateTransform();
	}

	void SetScale(const glm::vec3& scale) override
	{
		m_Scale = scale;
		UpdateTransform();
	}

	void SetPosition(const glm::vec3& position) override
	{
		m_Position = position;
		UpdateTransform();
	}

	const glm::mat4& GetFinal() const override
	{
		return m_FinalTransform.MODEL;
	}

	void SetFinal(const glm::mat4& transform) override
	{
		m_Position = KMath::ExtractPosition(transform);
		m_Rotate = glm::quat_cast(KMath::ExtractRotate(transform));
		m_Scale = KMath::ExtractScale(transform);
		UpdateTransform();
	}

	inline const KConstantDefinition::OBJECT& FinalTransform() const
	{
		return m_FinalTransform;
	}
};