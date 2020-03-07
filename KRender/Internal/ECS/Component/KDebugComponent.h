#pragma once
#include "KComponentBase.h"
#include "Internal/Asset/KMesh.h"

class KDebugComponent : public KComponentBase
{
protected:
	glm::vec4 m_Color;
public:
	KDebugComponent()
		: KComponentBase(CT_DEBUG),
		m_Color(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
	{
	}
	~KDebugComponent()
	{
	}

	inline const glm::vec4& Color() const { return m_Color; }
	inline void SetColor(const glm::vec4& color) { m_Color = color; }
};