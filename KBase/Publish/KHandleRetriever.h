#pragma once
#include <queue>

template<typename T>
class KHandleRetriever
{
protected:
	std::queue<T> m_RetrieveQueue;
	T m_HandleCounter;
public:
	KHandleRetriever()
		: m_HandleCounter(0)
	{}

	T NewHandle()
	{
		T handle;
		if (!m_RetrieveQueue.empty())
		{
			handle = m_RetrieveQueue.front();
			m_RetrieveQueue.pop();
		}
		else
		{
			handle = m_HandleCounter++;
		}
		return handle;
	}

	void ReleaseHandle(uint32_t handle)
	{
		m_RetrieveQueue.push(handle);
	}

	void Clear()
	{
		m_HandleCounter = 0;
		m_RetrieveQueue.swap(std::queue<T>());
	}

	bool Empty()
	{
		return (T)m_RetrieveQueue.size() == m_HandleCounter;
	}
};