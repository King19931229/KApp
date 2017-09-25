#pragma once
#include <queue>
#include "KSpinLock.h"

template<typename ElemType>
class KLockQueue
{
public:
	typedef std::queue<ElemType> QueueType;
protected:
	QueueType m_Queue;
	KSpinLock m_Lock;
	std::atomic_size_t m_uQueueSize;
public:
	KLockQueue()
	{
		m_uQueueSize = 0;
	}

	~KLockQueue()
	{
	}

	size_t Size()
	{
		return m_uQueueSize;
	}

	bool Empty()
	{
		return m_uQueueSize.load(std::memory_order_relaxed) == 0;
	}

	bool Pop(ElemType& elem)
	{
		{
			std::unique_lock<decltype(m_Lock)> lock(m_Lock);
			if(m_Queue.empty())
				return false;
			elem = m_Queue.front();
			m_Queue.pop();
		}
		m_uQueueSize.fetch_sub(1, std::memory_order_acq_rel);
		return true;
	}

	template<typename ProcFuncType>
	bool Pop(ElemType& elem, ProcFuncType func)
	{
		{
			std::unique_lock<decltype(m_Lock)> lock(m_Lock);
			if(m_Queue.empty())
				return false;
			elem = m_Queue.front();
			m_Queue.pop();
		}
		func(elem);
		m_uQueueSize.fetch_sub(1, std::memory_order_acq_rel);
		return true;
	}

	void Push(ElemType&& elem)
	{
		{
			std::unique_lock<decltype(m_Lock)> lock(m_Lock);
			m_Queue.push(elem);
		}
		m_uQueueSize.fetch_add(1, std::memory_order_acq_rel);
	}

	void Push(ElemType& elem)
	{
		{
			std::unique_lock<decltype(m_Lock)> lock(m_Lock);
			m_Queue.push(elem);
		}
		m_uQueueSize.fetch_add(1, std::memory_order_acq_rel);
	}

	void Clear()
	{
		{
			std::unique_lock<decltype(m_Lock)> lock(m_Lock);
			m_Queue.swap(QueueType());
		}
		m_uQueueSize.store(0, std::memory_order_release);
	}

	bool PopAll(QueueType& queue)
	{
		{
			std::unique_lock<decltype(m_Lock)> lock(m_Lock);
			queue.swap(QueueType());
			m_Queue.swap(queue);
		}
		m_uQueueSize.store(0, std::memory_order_release);
	}
};