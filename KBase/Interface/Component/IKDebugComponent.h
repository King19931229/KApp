#pragma once
#include "IKComponentBase.h"
#include "glm/glm.hpp"

struct IKDebugComponent : public IKComponentBase
{
	IKDebugComponent()
		: IKComponentBase(CT_DEBUG)
	{
	}
	virtual ~IKDebugComponent() {}

	virtual const glm::vec4& Color() const = 0;
	virtual void SetColor(const glm::vec4& color) = 0;
};