#pragma once
#include "KBase/Interface/Component/IKUserComponent.h"

// 先放在Render模块里
class KUserComponent : public IKUserComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKUserComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	TickFunction* m_PreTick;
	TickFunction* m_PostTick;
public:
	KUserComponent()
		: m_PreTick(nullptr)
		, m_PostTick(nullptr)
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

	void SetPreTick(TickFunction* preTick) override
	{
		m_PreTick = preTick;
	}

	void SetPostTick(TickFunction* postTick) override
	{
		m_PostTick = postTick;
	}

	bool PreTick() override
	{
		if (m_PreTick)
		{
			(*m_PreTick)();
		}
		return true;
	}

	bool PostTick() override
	{
		if (m_PostTick)
		{
			(*m_PostTick)();
		}
		return true;
	}
};