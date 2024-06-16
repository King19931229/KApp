#pragma once
#include <mutex>
#include <atomic>
#include <condition_variable>

class KEvent
{
protected:
	std::mutex m_Mutex;
	std::condition_variable m_CondVar;
	bool m_Signaled;
public:
	KEvent()
		: m_Signaled(false)
	{
	}

	~KEvent()
	{
	}

	bool Notify()
	{
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Signaled = true;
		}
		m_CondVar.notify_all();
		return true;
	}

	bool Wait()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);	
		m_CondVar.wait(lock, [&]() { return m_Signaled; });
		m_Signaled = false;
		return true;
	}

	bool WaitFor(int timeInMilliseconds)
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		if (!m_CondVar.wait_for(lock, std::chrono::milliseconds(timeInMilliseconds), [&]() { return m_Signaled; }))
		{
			return false;
		}
		m_Signaled = false;
		return true;
	}

	bool Reset()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Signaled = false;
	}
};