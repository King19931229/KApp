#pragma once
#include "KBase/Interface/Component/IKDebugComponent.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Publish/KMath.h"

class KDebugComponent : public IKDebugComponent
{
	RTTR_ENABLE(IKDebugComponent)
	RTTR_REGISTRATION_FRIEND
protected:
	glm::vec4 m_Color;
	static constexpr const char* msColor = "color";
public:
	KDebugComponent()
	{}
	~KDebugComponent()
	{}

	bool Save(IKXMLElementPtr element) override
	{
		IKXMLElementPtr colorEle = element->NewElement(msColor);
		std::string text;
		if (KMath::ToString(m_Color, text))
		{
			colorEle->SetText(text.c_str());
			return true;
		}
		return false;
	}

	bool Load(IKXMLElementPtr element) override
	{
		IKXMLElementPtr colorEle = element->FirstChildElement(msColor);
		if (colorEle && !colorEle->IsEmpty())
		{
			std::string text = colorEle->GetText();
			glm::vec4 color;
			if (KMath::FromString(text, color))
			{
				m_Color = color;
				return true;
			}
		}
		return false;
	}

	const glm::vec4& Color() const override { return m_Color; }
	void SetColor(const glm::vec4& color) override { m_Color = color; }
};