#pragma once
#include "IKComponentBase.h"
#include <functional>

struct IKUserComponent : public IKComponentBase
{
	RTTR_ENABLE(IKComponentBase)
	RTTR_REGISTRATION_FRIEND
public:
	IKUserComponent()
		: IKComponentBase(CT_USER)
	{
	}
	virtual ~IKUserComponent() {}

	typedef std::function<void(float dt)> TickFunction;

	virtual void SetTick(TickFunction tick) = 0;
};