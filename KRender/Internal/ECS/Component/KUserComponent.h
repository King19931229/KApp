#pragma once
#include "KBase/Interface/Component/IKUserComponent.h"

// 先放在Render模块里
class KUserComponent : public IKUserComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKUserComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	TickFunction m_Tick;
public:
	KUserComponent()
		: m_Tick(nullptr)
	{}

	virtual ~KUserComponent() {}

	bool Save(IKXMLElementPtr element) override
	{
		return true;
	}

	bool Load(IKXMLElementPtr element) override
	{
		return true;
	}

	void SetTick(TickFunction tick) override
	{
		m_Tick = tick;
	}

	bool Tick(float dt) override
	{
		if (m_Tick)
		{
			m_Tick(dt);
		}
		return true;
	}
};