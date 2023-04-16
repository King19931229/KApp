#pragma once
#include <atomic>

class KSectionEnterAssertGuard
{
protected:
	std::atomic_bool& m_Value;
public:
	KSectionEnterAssertGuard(std::atomic_bool& value)
		: m_Value(value)
	{
		assert(!m_Value && "assert failure please check");
		m_Value = true;
	}

	~KSectionEnterAssertGuard()
	{
		assert(m_Value && "assert failure please check");
		m_Value = false;
	}
};