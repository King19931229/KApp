#pragma once
#include "IKComponentBase.h"

struct IKRenderComponent : public IKComponentBase
{
	IKRenderComponent()
		: IKComponentBase(CT_RENDER)
	{
	}
	virtual ~IKRenderComponent() {}
};