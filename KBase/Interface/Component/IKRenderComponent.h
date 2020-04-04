#pragma once
#include "IKComponentBase.h"
#include "KBase/Publish/KAABBBox.h"

struct IKRenderComponent : public IKComponentBase
{
	IKRenderComponent()
		: IKComponentBase(CT_RENDER)
	{
	}
	virtual ~IKRenderComponent() {}
};