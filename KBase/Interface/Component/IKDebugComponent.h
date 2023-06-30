#pragma once
#include "IKComponentBase.h"
#include "KBase/Publish/KDebugUtility.h"

struct IKDebugComponent : public IKComponentBase
{
	RTTR_ENABLE(IKComponentBase)
	RTTR_REGISTRATION_FRIEND
public:
	IKDebugComponent()
		: IKComponentBase(CT_DEBUG)
	{
	}
	virtual ~IKDebugComponent() {}
};