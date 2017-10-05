#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

// 算法主体参考以下文章:
// Yet another implementation of a lock-free circular array queue
// https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circul
// sniperHW 翻译的这是中文译文:
// http://www.cnblogs.com/sniperHW/p/4172248.html
// 由于算法维持一个静态大小的RingBuffer由于RingBuffer大小固定
// 可能存在入队时RingBuffer已满的情况 因此维持一个缓冲队列WaitQueue

template<typename Elem_T, size_t ARRAY_SIZE = 1024>
class KLockFreeQueue
{
	typedef std::queue<Elem_T> WaitingQueue;
	typedef int ArraySizeTest[(ARRAY_SIZE & (ARRAY_SIZE - 1)) == 0 ? 1 : -1] ;
	typedef int ArraySizeTest2[(ARRAY_SIZE <= 32767) ? 1 : -1] ;
	inline short GetIndex(short nPos) { return nPos & (ARRAY_SIZE - 1); }

protected:
	Elem_T				m_RingBuffer[ARRAY_SIZE];
	WaitingQueue		m_WaitQueue;
	std::atomic_short	m_nReadIndex;
	std::atomic_short	m_nWriteIndex;
	std::atomic_short	m_nMaxReadIndex;
	std::atomic_short	m_nMaxWriteIndex;

	std::atomic_size_t	m_uQueueCount;
	std::mutex			m_WaitLock;

	size_t				m_uWaitQueueSize;

	void PushIntoWaitQueue(Elem_T& element)
	{
		std::lock_guard<decltype(m_WaitLock)> lock(m_WaitLock);
		m_WaitQueue.push(element);
		++m_uWaitQueueSize;
		assert(m_uWaitQueueSize == m_WaitQueue.size());
	}

	bool TryPush(Elem_T& element)
	{
		short nCurrentWriteIndex = -1;
		short nCurrentReadIndex = -1;
		short nNextWriteIndex = -1;

		while(true)
		{
			nCurrentWriteIndex	= m_nWriteIndex.load(std::memory_order_acquire);
			nCurrentReadIndex	= m_nReadIndex.load(std::memory_order_acquire);
			nNextWriteIndex = GetIndex(nCurrentWriteIndex + 1);

			if(nNextWriteIndex == m_nMaxWriteIndex)
				return false;

			if(m_nWriteIndex.compare_exchange_weak(nCurrentWriteIndex, nNextWriteIndex))
				break;
		}

		m_RingBuffer[nCurrentWriteIndex] = element;
		while(!m_nMaxReadIndex.compare_exchange_weak(nCurrentWriteIndex, nNextWriteIndex))
		{
			std::this_thread::yield();
		}
		m_uQueueCount.fetch_add(1, std::memory_order_acq_rel);
		return true;
	}
public:
	KLockFreeQueue()
	{
		m_nReadIndex = 0;
		m_nWriteIndex = 0;
		m_nMaxReadIndex = 0;
		m_nMaxWriteIndex = 0;
		m_uQueueCount = 0;
		m_uWaitQueueSize = 0;
	}

	~KLockFreeQueue()
	{

	}

	size_t Size()
	{
		return m_uQueueCount.load(std::memory_order_relaxed) + m_uWaitQueueSize;
	}

	size_t WaitQueueSize()
	{
		return m_uWaitQueueSize;
	}

	bool Empty()
	{
		return Size() == 0;
	}

	bool Push(Elem_T&& element)
	{
		if(m_uWaitQueueSize || !TryPush(element))
			PushIntoWaitQueue(element);
		return true;
	}

	bool Push(Elem_T& element)
	{
		if(m_uWaitQueueSize || !TryPush(element))
			PushIntoWaitQueue(element);
		return true;
	}

	bool Pop(Elem_T& element)
	{
		short nCurrentReadIndex = -1;
		short nCurrentMaxReadIndex = -1;
		short nNextReadIndex = -1;
		bool bRet = false;
		while(true)
		{
			nCurrentReadIndex		= m_nReadIndex.load(std::memory_order_acquire);
			nCurrentMaxReadIndex	= m_nMaxReadIndex.load(std::memory_order_acquire);
			nNextReadIndex			= GetIndex(m_nReadIndex + 1);
			if(nCurrentReadIndex == nCurrentMaxReadIndex)
				break;
			if(m_nReadIndex.compare_exchange_weak(nCurrentReadIndex, nNextReadIndex))
			{
				element = m_RingBuffer[nCurrentReadIndex];

				while(!m_nMaxWriteIndex.compare_exchange_weak(nCurrentReadIndex, nNextReadIndex))
				{
					std::this_thread::yield();
				}

				m_uQueueCount.fetch_sub(1, std::memory_order_acq_rel);
				bRet = true;
				break;
			}
		}
		if(bRet)
			FlushWaitingElement();
		return bRet;
	}

	template<typename ProcFuncType>
	bool Pop(Elem_T& element, ProcFuncType func)
	{
		short nCurrentReadIndex = -1;
		short nCurrentMaxReadIndex = -1;
		short nNextReadIndex = -1;
		bool bRet = false;
		while(true)
		{
			nCurrentReadIndex		= m_nReadIndex.load(std::memory_order_acquire);
			nCurrentMaxReadIndex	= m_nMaxReadIndex.load(std::memory_order_acquire);
			nNextReadIndex			= GetIndex(m_nReadIndex + 1);
			if(nCurrentReadIndex == nCurrentMaxReadIndex)
				break;
			if(m_nReadIndex.compare_exchange_weak(nCurrentReadIndex, nNextReadIndex))
			{
				element = m_RingBuffer[nCurrentReadIndex];
				func(element);

				while(!m_nMaxWriteIndex.compare_exchange_weak(nCurrentReadIndex, nNextReadIndex))
				{
					std::this_thread::yield();
				}

				m_uQueueCount.fetch_sub(1, std::memory_order_acq_rel);
				bRet = true;
				break;
			}
		}
		if(bRet)
			FlushWaitingElement();
		return bRet;
	}

	void FlushWaitingElement()
	{
		while(true)
		{
			if(m_uWaitQueueSize)
			{
				std::lock_guard<decltype(m_WaitLock)> lock(m_WaitLock);
				if(m_uWaitQueueSize && TryPush(m_WaitQueue.front()))
				{
					m_WaitQueue.pop();
					--m_uWaitQueueSize;
					assert(m_uWaitQueueSize == m_WaitQueue.size());
					if(m_uWaitQueueSize == 0)
						break;
				}
				else
					break;
			}
			else
				break;
		}
	}
};