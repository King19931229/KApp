#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

// Yet another implementation of a lock-free circular array queue
// https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circul
// 中文译文:
// http://www.cnblogs.com/sniperHW/p/4172248.html

// C++11 CAS
// http://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange
// 注意C++11中atomic的CAS操作中 【Exp】是以【引用】传入而非【常引用】
// 所以标准就允许CAS操作后【Exp】被修改
// VS2012下CAS后【Exp】就有被修改的可能

// TODO BUG IN ARRAY_SIZE = 2
template<typename Elem_T, size_t ARRAY_SIZE = 2048>
class KLockFreeQueue
{
	typedef std::queue<Elem_T> WaitingQueue;
	typedef int ArraySizeTest[(ARRAY_SIZE & (ARRAY_SIZE - 1)) == 0 ? 1 : -1] ;
	typedef int ArraySizeTest2[(ARRAY_SIZE <= 0x7FFF) ? 1 : -1] ;
	typedef int ArraySizeTest3[(ARRAY_SIZE > 0x0002) ? 1 : -1] ;
	inline short GetIndex(short nPos) { return nPos & (ARRAY_SIZE - 1); }
protected:
	Elem_T				m_RingBuffer[ARRAY_SIZE];
	WaitingQueue		m_WaitQueue;
	std::atomic_short	m_nReadIndex;
	std::atomic_short	m_nWriteIndex;
	std::atomic_short	m_nMaxReadIndex;
	std::atomic_short	m_nMaxWriteIndex;
	std::atomic_size_t	m_uWaitQueueSize;
	std::mutex			m_WaitLock;

	void PushIntoWaitQueue(Elem_T& element)
	{
		std::lock_guard<decltype(m_WaitLock)> lock(m_WaitLock);
		++m_uWaitQueueSize;
		m_WaitQueue.push(element);
		assert(m_uWaitQueueSize == m_WaitQueue.size());
	}

	bool TryPush(Elem_T& element)
	{
		short nCurrentWriteIndex = -1;
		short nNextWriteIndex = -1;
		short nExp = -1;

		do
		{
			nCurrentWriteIndex	= m_nWriteIndex.load(std::memory_order_relaxed);
			nNextWriteIndex		= GetIndex(nCurrentWriteIndex + 1);
			// C++11 CAS操作会修改【Exp】
			nExp				= nCurrentWriteIndex;
			// 队列已满不能【Push】
			if(nNextWriteIndex == m_nMaxWriteIndex.load(std::memory_order_relaxed))
				return false;
		}while(!m_nWriteIndex.compare_exchange_weak(nExp, nNextWriteIndex, std::memory_order_seq_cst));

		m_RingBuffer[nCurrentWriteIndex] = element;
		// C++11 CAS操作会修改【Exp】
		nExp = nCurrentWriteIndex;
		// 这是【写保护】防止【读操作】把未在队列中的元素读出
		// 只有最小Compare成功的WriteIndex可以修改到MaxReadIndex 保证MaxReadIndex正确性
		while(!m_nMaxReadIndex.compare_exchange_weak(nExp, nNextWriteIndex, std::memory_order_seq_cst))
		{
			// C++11 CAS操作会修改【Exp】
			nExp = nCurrentWriteIndex;
			// 多个线程同时【Push】产生冲突 当前线程【让出】
			std::this_thread::yield();
		}
		return true;
	}
public:
	KLockFreeQueue()
	{
		m_nReadIndex = 0;
		m_nWriteIndex = 0;
		m_nMaxReadIndex = 0;
		m_nMaxWriteIndex = 0;
		m_uWaitQueueSize = 0;
	}

	~KLockFreeQueue()
	{

	}

	size_t Size()
	{
		return RingElementSize() + WaitQueueSize();
	}

	inline size_t WaitQueueSize()
	{
		return m_uWaitQueueSize;
	}

	inline size_t RingElementSize()
	{
		short nMaxWriteIndex = m_nMaxWriteIndex.load(std::memory_order_relaxed);
		short nMaxReadIndex = m_nMaxReadIndex.load(std::memory_order_relaxed);
		if(nMaxWriteIndex >= nMaxReadIndex)
			return (size_t)(nMaxWriteIndex - nMaxReadIndex);
		else
			return (size_t)(ARRAY_SIZE + nMaxWriteIndex - nMaxReadIndex);
	}

	bool Empty()
	{
		return Size() == 0;
	}

	bool Push(Elem_T&& element)
	{
		// 如果WaitQueue不为空就不要直接Push到无锁队列里
		// 不然Pop的时候把WaitQueue的元素再Push到无锁队列时候
		// 容易发生死锁
		if(m_uWaitQueueSize || !TryPush(element))
		{
			PushIntoWaitQueue(element);
		}
		return true;
	}

	bool Push(Elem_T& element)
	{
		if(m_uWaitQueueSize || !TryPush(element))
		{
			PushIntoWaitQueue(element);
		}
		return true;
	}

	bool TryPop(Elem_T& element)
	{
		bool bRet = false;
		short nCurrentReadIndex = -1;
		short nNextReadIndex = -1;
		do
		{
			nCurrentReadIndex		= m_nReadIndex.load(std::memory_order_relaxed);
			nNextReadIndex			= GetIndex(nCurrentReadIndex + 1);
			// C++11 CAS操作会修改【Exp】
			nExp					= nCurrentReadIndex;
			if(nCurrentReadIndex == m_nMaxReadIndex.load(std::memory_order_relaxed))
				break;
			if(m_nReadIndex.compare_exchange_stromg(nExp, nNextReadIndex, std::memory_order_seq_cst))
			{
				element = m_RingBuffer[nCurrentReadIndex];
				// C++11 CAS操作会修改【Exp】
				nExp = nCurrentReadIndex;
				// 这是【读保护】防止【写操作】把正在队列中的元素覆盖掉
				// 只有最小Compare成功的ReadIndex可以修改到MaxWriteIndex 保证MaxWriteIndex正确性
				while(!m_nMaxWriteIndex.compare_exchange_strong(nExp, nNextReadIndex, std::memory_order_seq_cst))
				{
					// C++11 CAS操作会修改【Exp】
					nExp = nCurrentReadIndex;
					// 多个线程同时【Pop】产生冲突 当前线程【让出】
					std::this_thread::yield();
				}
				m_nQueueCount.fetch_sub(1, std::memory_order_acq_rel);
				// 把未在环形数组的元素传入
				FlushWaitingElement();
				return true;
			}
		}while(true);
		// 把未在环形数组的元素传入
		FlushWaitingElement();
		return false;
	}

	bool Pop(Elem_T& element)
	{
		if(TryPop(element))
		{
			return true;
		}
		return false;
	}

	template<typename ProcFuncType>
	bool TryPop(Elem_T& element, ProcFuncType func)
	{
		short nCurrentReadIndex = -1;
		short nNextReadIndex = -1;
		short nExp = -1;
		do
		{
			nCurrentReadIndex		= m_nReadIndex.load(std::memory_order_relaxed);
			nNextReadIndex			= GetIndex(nCurrentReadIndex + 1);
			// C++11 CAS操作会修改【Exp】
			nExp					= nCurrentReadIndex;
			if(nCurrentReadIndex == m_nMaxReadIndex.load(std::memory_order_relaxed))
				break;
			if(m_nReadIndex.compare_exchange_strong(nExp, nNextReadIndex, std::memory_order_seq_cst))
			{
				element = m_RingBuffer[nCurrentReadIndex];
				// C++11 CAS操作会修改【Exp】
				nExp = nCurrentReadIndex;
				// 这是【读保护】防止【写操作】把正在队列中的元素覆盖掉
				// 只有最小Compare成功的ReadIndex可以修改到MaxWriteIndex 保证MaxWriteIndex正确性
				while(!m_nMaxWriteIndex.compare_exchange_strong(nExp, nNextReadIndex, std::memory_order_seq_cst))
				{
					// C++11 CAS操作会修改【Exp】
					nExp = nCurrentReadIndex;
					// 多个线程同时【Pop】产生冲突 当前线程【让出】
					std::this_thread::yield();
				}
				func(element);
				// 把未在环形数组的元素传入
				FlushWaitingElement();
				return true;
			}
		}while(true);
		// 把未在环形数组的元素传入
		FlushWaitingElement();
		return false;
	}

	template<typename ProcFuncType>
	bool Pop(Elem_T& element, ProcFuncType func)
	{
		if(TryPop(element, func))
		{
			return true;
		}
		return false;
	}

	void FlushWaitingElement()
	{
		if(m_uWaitQueueSize)
		{
			std::lock_guard<decltype(m_WaitLock)> lock(m_WaitLock);
			if(m_uWaitQueueSize)
			{
				while(!TryPush(m_WaitQueue.front()));
				m_WaitQueue.pop();
				--m_uWaitQueueSize;
			}
		}
	}
};