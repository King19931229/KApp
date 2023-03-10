#include "KTransformComponent.h"

RTTR_REGISTRATION
{
#define KRTTR_REG_CLASS_NAME KTransformComponent
#define KRTTR_REG_CLASS_NAME_STR "TransformComponent"

	KRTTR_REG_CLASS_BEGIN()
	KRTTR_REG_PROPERTY_GET_SET_NOTIFY(position, GetPosition, SetPosition, MDT_FLOAT3, MDN_EDITOR)
	KRTTR_REG_PROPERTY_GET_SET_NOTIFY(scale, GetScale, SetScale, MDT_FLOAT3, MDN_EDITOR)
	KRTTR_REG_PROPERTY_GET_SET_NOTIFY(rotate, GetRotateEularAngles, SetRotateEularAngles, MDT_FLOAT3, MDN_EDITOR)
	KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
}

KTransformComponent::KTransformComponent()
	: m_Position(glm::vec3(0.0f)),
	m_Scale(glm::vec3(1.0f)),
	m_Rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
	m_IsStatic(false),
	m_TransformChange(false)
{
	m_FinalTransform.MODEL = glm::mat4(1.0f);
	m_FinalTransform.PRVE_MODEL = glm::mat4(1.0f);
}

KTransformComponent:: ~KTransformComponent() {}

bool KTransformComponent::RegisterTransformChangeCallback(KTransformChangeCallback* callback)
{
	if (callback)
	{
		auto it = std::find(m_TransformChangeCallbacks.begin(), m_TransformChangeCallbacks.end(), callback);
		if (it == m_TransformChangeCallbacks.end())
		{
			m_TransformChangeCallbacks.push_back(callback);
		}
		return true;
	}
	return false;
}

bool KTransformComponent::UnRegisterTransformChangeCallback(KTransformChangeCallback* callback)
{
	if (callback)
	{
		auto it = std::find(m_TransformChangeCallbacks.begin(), m_TransformChangeCallbacks.end(), callback);
		if (it == m_TransformChangeCallbacks.end())
		{
			m_TransformChangeCallbacks.erase(it);
		}
		return true;
	}
	return false;
}