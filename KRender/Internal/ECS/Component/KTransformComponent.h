#pragma once
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Publish/KMath.h"
#include "Internal/KConstantDefinition.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

#include "KBase/Publish/KMath.h"

class KTransformComponent : public IKTransformComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKTransformComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	glm::vec3 m_Position;
	glm::vec3 m_Scale;
	glm::quat m_Rotate;	
	bool m_IsStatic;
	bool m_TransformChange;
	
	KConstantDefinition::OBJECT m_FinalTransform;
	std::vector<KTransformChangeCallback*> m_TransformChangeCallbacks;

	static constexpr const char* msPosition = "position";
	static constexpr const char* msScale = "scale";
	static constexpr const char* msRotate = "rotate";
	static constexpr const char* msIsStatic = "static";

	void UpdateTransform()
	{
		glm::mat4 rotate = glm::mat4_cast(m_Rotate);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);

		m_FinalTransform.MODEL = translate * rotate * scale;
		m_TransformChange = true;
	}
public:
	KTransformComponent();
	virtual ~KTransformComponent();

	bool Save(IKXMLElementPtr element) override
	{
		std::string pos;
		ACTION_ON_FAILURE(KMath::ToString(m_Position, pos), return false);

		std::string scale;
		ACTION_ON_FAILURE(KMath::ToString(m_Scale, scale), return false);

		std::string rorate;
		ACTION_ON_FAILURE(KMath::ToString(m_Rotate, rorate), return false);

		IKXMLElementPtr posEle = element->NewElement(msPosition);
		posEle->SetText(pos.c_str());

		IKXMLElementPtr scaleEle = element->NewElement(msScale);
		scaleEle->SetText(scale.c_str());

		IKXMLElementPtr rotateEle = element->NewElement(msRotate);
		rotateEle->SetText(rorate.c_str());

		IKXMLElementPtr staticEle = element->NewElement(msIsStatic);
		staticEle->SetText(m_IsStatic);

		return true;
	}

	bool Load(IKXMLElementPtr element) override
	{
		IKXMLElementPtr posEle = element->FirstChildElement(msPosition);
		ACTION_ON_FAILURE(posEle != nullptr && !posEle->IsEmpty(), return false);

		IKXMLElementPtr scaleEle = element->FirstChildElement(msScale);
		ACTION_ON_FAILURE(scaleEle != nullptr && !scaleEle->IsEmpty(), return false);

		IKXMLElementPtr rotateEle = element->FirstChildElement(msRotate);
		ACTION_ON_FAILURE(rotateEle != nullptr && !rotateEle->IsEmpty(), return false);

		IKXMLElementPtr staticEle = element->FirstChildElement(msIsStatic);
		ACTION_ON_FAILURE(staticEle != nullptr && !staticEle->IsEmpty(), return false);

		glm::vec3 pos;
		ACTION_ON_FAILURE(KMath::FromString(posEle->GetText(), pos), return false);
		glm::vec3 scale;
		ACTION_ON_FAILURE(KMath::FromString(scaleEle->GetText(), scale), return false);
		glm::quat rotate;
		ACTION_ON_FAILURE(KMath::FromString(rotateEle->GetText(), rotate), return false);

		ACTION_ON_FAILURE(KStringParser::ParseToBOOL(staticEle->GetText().c_str(), &m_IsStatic, 1));

		m_Position = pos;
		m_Scale = scale;
		m_Rotate = rotate;

		UpdateTransform();

		return true;
	}

	bool PostTick() override
	{
		m_FinalTransform.PRVE_MODEL = m_FinalTransform.MODEL;
		if (m_TransformChange)
		{
			for (KTransformChangeCallback* callback : m_TransformChangeCallbacks)
			{
				(*callback)(this, m_Position, m_Scale, m_Rotate);
			}
		}
		m_TransformChange = false;
		return true;
	}

	bool RegisterTransformChangeCallback(KTransformChangeCallback* callback) override;
	bool UnRegisterTransformChangeCallback(KTransformChangeCallback* callback) override;

	const glm::quat& GetRotateQuat() const override { return m_Rotate; }
	const glm::vec3& GetScale() const override { return m_Scale; }
	const glm::vec3& GetPosition() const override { return m_Position; }

	glm::vec3 GetRotateEularAngles() const override
	{ 
		return glm::eulerAngles(m_Rotate) * 180.0f / glm::pi<float>();
	}

	bool IsStatic() const override { return m_IsStatic; }

	void SetRotateQuat(const glm::quat& rotate) override
	{
		if (!m_IsStatic)
		{
			m_Rotate = rotate;
			UpdateTransform();
		}
	}

	void SetRotateMatrix(const glm::mat3& rotate) override
	{
		if (!m_IsStatic)
		{
			m_Rotate = glm::quat_cast(rotate);
			UpdateTransform();
		}
	}

	void SetRotateEularAngles(glm::vec3 eularAngles) override
	{
		if (!m_IsStatic)
		{
			m_Rotate = glm::quat(eularAngles * glm::pi<float>() / 180.0f);
			UpdateTransform();
		}
	}

	void SetScale(const glm::vec3& scale) override
	{
		if (!m_IsStatic)
		{
			m_Scale = scale;
			UpdateTransform();
		}
	}

	void SetPosition(const glm::vec3& position) override
	{
		if (!m_IsStatic)
		{
			m_Position = position;
			UpdateTransform();
		}
	}

	const glm::mat4& GetFinal() const override
	{
		return m_FinalTransform.MODEL;
	}

	const glm::mat4& GetPrevFinal() const override
	{
		return m_FinalTransform.PRVE_MODEL;
	}

	void SetFinal(const glm::mat4& transform) override
	{
		m_Position = KMath::ExtractPosition(transform);
		m_Rotate = glm::quat_cast(KMath::ExtractRotate(transform));
		m_Scale = KMath::ExtractScale(transform);
		UpdateTransform();
	}

	void SetStatic(bool isStatic) override
	{
		m_IsStatic = isStatic;
	}

	inline const KConstantDefinition::OBJECT& FinalTransform() const
	{
		return m_FinalTransform;
	}
};