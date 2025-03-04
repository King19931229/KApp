#pragma once
#include <mutex>
#include <atomic>
#include <condition_variable>

class KSemaphore
{
protected:
	int m_nCount;
	std::mutex m_Mutex;
	std::condition_variable m_CondVar;
public:
	KSemaphore()
		: m_nCount(0)
	{
	}

	~KSemaphore()
	{
	}

	bool Notify()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		if(++m_nCount <= 0)
		{
			m_CondVar.notify_one();
			return true;
		}
		return false;
	}

	bool NotifyAll()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		if(++m_nCount <= 0)
		{
			m_CondVar.notify_all();
			return true;
		}
		return false;
	}

	bool Wait()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		if(--m_nCount < 0)
		{
			m_CondVar.wait(lock);
		}
		return true;
	}

	bool WaitFor(int timeInMilliseconds)
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		if(--m_nCount < 0)
		{
			if(m_CondVar.wait_for(lock, std::chrono::milliseconds(timeInMilliseconds)) == std::cv_status::timeout)
			{
				++m_nCount;
				return false;
			}
		}
		return true;
	}

	bool Reset()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_nCount = 0;
		return true;
	}

	int GetCount()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		return m_nCount;
	}
};