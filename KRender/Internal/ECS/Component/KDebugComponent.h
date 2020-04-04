#pragma once
#include "KBase/Interface/Component/IKDebugComponent.h"
#include "Internal/Asset/KMesh.h"

class KDebugComponent : public IKDebugComponent
{
protected:
	glm::vec4 m_Color;
public:
	KDebugComponent()
	{}
	~KDebugComponent()
	{}

	const glm::vec4& Color() const override { return m_Color; }
	void SetColor(const glm::vec4& color) override { m_Color = color; }
};