#pragma once
#include <atomic>
class KSpinLock
{
protected:
	std::atomic_flag m_Flag;
public:
	KSpinLock()
		: m_Flag()
	{
		m_Flag.clear();
	}
	void lock()
	{
		while(m_Flag.test_and_set(std::memory_order_acquire));
	}
	void unlock()
	{
		m_Flag.clear(std::memory_order_release);
	}
};