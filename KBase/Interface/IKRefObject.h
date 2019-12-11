#pragma once
#include <atomic>

class IKRefObject
{
protected:
	std::atomic_int m_Ref;
public:
	IKRefObject()
	{
		m_Ref = 1;
	}
	virtual ~IKRefObject() = 0;
	void AddRef() { m_Ref += 1; }
	void Release()
	{
		m_Ref -= 1;
		if(m_Ref == 0)
		{
			delete this;
		}
	}
};