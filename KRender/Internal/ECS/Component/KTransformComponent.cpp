#include "KTransformComponent.h"
#include "Internal/KRenderThread.h"

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
	: m_RenderScene(nullptr),
	m_Position(glm::vec3(0.0f)),
	m_Scale(glm::vec3(1.0f)),
	m_Rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
	m_IsStatic(false)
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

bool KTransformComponent::Save(IKXMLElementPtr element)
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

bool KTransformComponent::Load(IKXMLElementPtr element)
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

void KTransformComponent::UpdateTransform()
{
	ASSERT_RESULT(IsInGameThread());
	glm::mat4 rotate = glm::mat4_cast(m_Rotate);
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
	glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);

	m_FinalTransform.MODEL = translate * rotate * scale;

	if (memcmp(&m_FinalTransform_RenderThread, &m_FinalTransform.MODEL, sizeof(m_FinalTransform_RenderThread)) != 0)
	{
		ENQUEUE_RENDER_COMMAND(KTransformComponent_SyncTransformToRender)([this, transform = m_FinalTransform, pos = m_Position, scl = m_Scale, rot = m_Rotate]()
		{
			m_FinalTransform_RenderThread = transform;
			for (KTransformChangeCallback* callback : m_TransformChangeCallbacks)
			{
				(*callback)(this, pos, scl, rot);
			}
		});

		if (m_RenderScene)
		{
			m_RenderScene->Transform(GetEntityHandle());
		}
	}
}

bool KTransformComponent::Tick(float dt)
{
	ASSERT_RESULT(IsInGameThread());
	m_FinalTransform.PRVE_MODEL = m_FinalTransform.MODEL;
	return true;
}

const glm::quat& KTransformComponent::GetRotateQuat() const 
{
	ASSERT_RESULT(IsInGameThread());
	return m_Rotate;
}

const glm::vec3& KTransformComponent::GetScale() const 
{
	ASSERT_RESULT(IsInGameThread());
	return m_Scale;
}

const glm::vec3& KTransformComponent::GetPosition() const
{
	ASSERT_RESULT(IsInGameThread());
	return m_Position;
}

glm::vec3 KTransformComponent::GetRotateEularAngles() const 
{
	ASSERT_RESULT(IsInGameThread());
	return glm::eulerAngles(m_Rotate) * 180.0f / glm::pi<float>();
}

bool KTransformComponent::IsStatic() const
{
	return m_IsStatic;
}

void KTransformComponent::SetRotateQuat(const glm::quat& rotate) 
{
	ASSERT_RESULT(IsInGameThread());
	if (!m_IsStatic)
	{
		m_Rotate = rotate;
		UpdateTransform();
	}
}

void KTransformComponent::SetRotateMatrix(const glm::mat3& rotate) 
{
	ASSERT_RESULT(IsInGameThread());
	if (!m_IsStatic)
	{
		m_Rotate = glm::quat_cast(rotate);
		UpdateTransform();
	}
}

void KTransformComponent::SetRotateEularAngles(glm::vec3 eularAngles) 
{
	ASSERT_RESULT(IsInGameThread());
	if (!m_IsStatic)
	{
		m_Rotate = glm::quat(eularAngles * glm::pi<float>() / 180.0f);
		UpdateTransform();
	}
}

void KTransformComponent::SetScale(const glm::vec3& scale) 
{
	ASSERT_RESULT(IsInGameThread());
	if (!m_IsStatic)
	{
		m_Scale = scale;
		UpdateTransform();
	}
}

void KTransformComponent::SetPosition(const glm::vec3& position) 
{
	ASSERT_RESULT(IsInGameThread());
	if (!m_IsStatic)
	{
		m_Position = position;
		UpdateTransform();
	}
}

const glm::mat4& KTransformComponent::GetFinal_GameThread() const 
{
	ASSERT_RESULT(IsInGameThread());
	return m_FinalTransform.MODEL;
}

const glm::mat4& KTransformComponent::GetPrevFinal_GameThread() const 
{
	ASSERT_RESULT(IsInGameThread());
	return m_FinalTransform.PRVE_MODEL;
}

const glm::mat4& KTransformComponent::GetFinal_RenderThread() const 
{
	ASSERT_RESULT(IsInRenderThread());
	return m_FinalTransform_RenderThread.MODEL;
}

const glm::mat4& KTransformComponent::GetPrevFinal_RenderThread() const 
{
	ASSERT_RESULT(IsInRenderThread());
	return m_FinalTransform_RenderThread.PRVE_MODEL;
}

const glm::mat4& KTransformComponent::GetFinal() const
{
	if (IsInGameThread())
	{
		return GetFinal_GameThread();
	}
	return GetFinal_RenderThread();
}

const glm::mat4& KTransformComponent::GetPrevFinal() const
{
	if (IsInGameThread())
	{
		return GetPrevFinal_GameThread();
	}
	return GetPrevFinal_RenderThread();
}

void KTransformComponent::SetFinal(const glm::mat4& transform) 
{
	ASSERT_RESULT(IsInGameThread());
	m_Position = KMath::ExtractPosition(transform);
	m_Rotate = glm::quat_cast(KMath::ExtractRotate(transform));
	m_Scale = KMath::ExtractScale(transform);
	UpdateTransform();
}

void KTransformComponent::SetStatic(bool isStatic) 
{
	ASSERT_RESULT(IsInGameThread());
	m_IsStatic = isStatic;
}

void KTransformComponent::SetRenderScene(IKRenderScene* renderScene)
{
	m_RenderScene = renderScene;
}

const KConstantDefinition::OBJECT& KTransformComponent::FinalTransform_RenderThread() const
{
	ASSERT_RESULT(IsInRenderThread());
	return m_FinalTransform;
}