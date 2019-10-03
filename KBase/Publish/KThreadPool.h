#pragma once
#include "Publish/KLockFreeQueue.h"
#include "Publish/KLockQueue.h"
#include "Publish/KSemaphore.h"
#include "Publish/KThreadTool.h"

template<typename Task, bool bUseLockFreeQueue = false>
class KThreadPool
{
	struct TaskGroup
	{
		bool bHasSyncTask;
		Task asyncTask;
		Task syncTask;

		TaskGroup()
		{
			bHasSyncTask = false;
		}
	};

	template<bool bUseLockFreeQueue>
	struct BOOLToTYPE
	{
	};

	template<>
	struct BOOLToTYPE<false>
	{
		typedef KLockQueue<TaskGroup> QueueType;
	};

	template<>
	struct BOOLToTYPE<true>
	{
		typedef KLockFreeQueue<TaskGroup> QueueType;
	};

	typedef typename BOOLToTYPE<bUseLockFreeQueue>::QueueType QueueType;
	typedef std::vector<Task> SharedQueueType;

	struct TripleQueue
	{
		SharedQueueType asyncQueue;
		SharedQueueType async_Sync_Swap;
		SharedQueueType syncQueue;
		std::mutex		lock;
		KSemaphore		sem;
		bool			bSwapNoEmpty;

		TripleQueue()
		{
			bSwapNoEmpty = false;
		}
	};

	////////////////////////////////////////////////////////////
	class KWorkerThread
	{
	protected:
		QueueType& m_Queue;
		TripleQueue& m_SharedQueue;

		bool m_bDone;
		KSemaphore m_Sem;
		KSemaphore m_SemForEmtpy;
		std::thread m_Thread;

		static unsigned short ms_IDCounter;
		static std::mutex ms_IDLock;
		static std::queue<unsigned short> ms_IDQueue;

		void ThreadFunc()
		{
			unsigned short uID = 0;
			{
				std::lock_guard<decltype(ms_IDLock)> lockGuard(ms_IDLock);
				if(!ms_IDQueue.empty())
				{
					uID = ms_IDQueue.front();
					ms_IDQueue.pop();
				}
				else
				{
					uID = ms_IDCounter++;
				}
				char szBuffer[256]; szBuffer[0] = '\0';
#ifndef _WIN32
				snprintf(szBuffer, sizeof(szBuffer), "WorkerThread %d", uID);
#else
				sprintf_s(szBuffer, sizeof(szBuffer), "WorkerThread %d", uID);
#endif
				KThreadTool::SetThreadName(szBuffer);
			}

			TaskGroup taskGroup;
			while(!m_bDone)
			{
				if(m_Queue.Empty() && !m_bDone)
					m_Sem.Wait();

				if(m_bDone)
					break;

				m_Queue.Pop(taskGroup, [this](TaskGroup& taskGroup)
				{
					taskGroup.asyncTask();
					if(taskGroup.bHasSyncTask)
					{
						std::unique_lock<decltype(m_SharedQueue.lock)> lock(m_SharedQueue.lock);
						m_SharedQueue.asyncQueue.push_back(taskGroup.syncTask);
						m_SharedQueue.sem.Notify();
					}
				});

				m_SemForEmtpy.Notify();
			}

			{
				std::lock_guard<decltype(ms_IDLock)> lockGuard(ms_IDLock);
				ms_IDQueue.push(uID);
			}
		}
	public:
		KWorkerThread(QueueType& queue,
			TripleQueue& sharedQueue)
			: m_Queue(queue),
			m_SharedQueue(sharedQueue),
			m_bDone(false),
			m_Thread(&KWorkerThread::ThreadFunc, this)
		{
		}

		~KWorkerThread()
		{
			m_bDone = true;
			m_Sem.Notify();
			m_Thread.join();
		}

		bool Notify()
		{
			return m_Sem.Notify();
		}

		bool Wait()
		{
			return m_SemForEmtpy.WaitUntil([&]() { return m_Queue.Empty(); });
		}
	};
	////////////////////////////////////////////////////////////
	class KSyncTaskThread
	{
	protected:
		TripleQueue& m_SharedQueue;

		bool m_bDone;
		std::thread m_Thread;

		void ThreadFunc()
		{
			KThreadTool::SetThreadName("SyncTaskThread");
			bool bNeedWait = false;
			while(!m_bDone)
			{
				bNeedWait = false;

				if(m_bDone)
					break;

				{
					std::unique_lock<decltype(m_SharedQueue.lock)> lock(m_SharedQueue.lock);
					if(m_SharedQueue.async_Sync_Swap.empty())
					{
						if(m_SharedQueue.asyncQueue.empty())
						{
							bNeedWait = true;
						}
						else
						{
							m_SharedQueue.async_Sync_Swap.swap(m_SharedQueue.asyncQueue);
							m_SharedQueue.bSwapNoEmpty = true;
						}
					}
				}

				if(!m_bDone && bNeedWait)
					m_SharedQueue.sem.Wait();
			}
		}
	public:
		KSyncTaskThread(TripleQueue& sharedQueue)
			: m_SharedQueue(sharedQueue),
			m_bDone(false),
			m_Thread(&KSyncTaskThread::ThreadFunc, this)
		{
		}

		~KSyncTaskThread()
		{
			m_bDone = true;
			m_SharedQueue.sem.Notify();
			m_Thread.join();
		}

		bool Notify()
		{
			return m_SharedQueue.sem.Notify();
		}
	};
	////////////////////////////////////////////////////////////
protected:
	KSemaphore m_Sem;
	QueueType m_Queue;
	TripleQueue m_SharedQueue;

	std::vector<KWorkerThread*> m_Threads;
	KSyncTaskThread *m_pSyncTaskThread;
public:
	KThreadPool()
		: m_pSyncTaskThread(nullptr)
	{
		m_pSyncTaskThread = new KSyncTaskThread(m_SharedQueue);
		assert(m_pSyncTaskThread);
	}

	~KThreadPool()
	{
		for(auto it = m_Threads.begin(), itEnd = m_Threads.end();
			it != itEnd; ++it)
		{
			delete *it;
			*it = nullptr;
		}
		m_Threads.clear();

		delete m_pSyncTaskThread;
		m_pSyncTaskThread = nullptr;
	}

	void PushWorkerThreads(size_t uThreadNum)
	{
		while(uThreadNum--)
		{
			KWorkerThread* pThread = new KWorkerThread(m_Queue, m_SharedQueue);
			assert(pThread);
			m_Threads.push_back(pThread);
		}
	}

	void PopWorkerThreads(size_t uThreadNum)
	{
		assert(uThreadNum <= m_Threads.size());
		uThreadNum = uThreadNum > m_Threads.size() ? m_Threads.size() : uThreadNum;
		while(uThreadNum--)
		{
			KWorkerThread* pThread = m_Threads.back();
			m_Threads.pop_back();
			delete pThread;
			pThread = nullptr;
		}
	}

	void PopAllWorkerThreads()
	{
		PopWorkerThreads(m_Threads.size());
	}

	size_t GetWorkerThreadNum()
	{
		return m_Threads.size();
	}

	void SubmitTask(Task asyncTask)
	{
		TaskGroup group;
		group.asyncTask = asyncTask;

		m_Queue.Push(group);
		std::find_if(m_Threads.begin(), m_Threads.end(), std::mem_fun(&KWorkerThread::Notify));
	}

	void SubmitTask(Task asyncTask, Task syncTask)
	{
		TaskGroup group;

		group.asyncTask = asyncTask;
		group.syncTask = syncTask;
		group.bHasSyncTask = true;

		m_Queue.Push(group);
		std::find_if(m_Threads.begin(), m_Threads.end(), std::mem_fun(&KWorkerThread::Notify));
	}

	bool AllAsyncTaskDone()
	{
		return m_Queue.Empty();
	}

	bool AllSyncTaskDone()
	{
		std::unique_lock<decltype(m_SharedQueue.lock)> lock(m_SharedQueue.lock);
		return m_SharedQueue.syncQueue.empty() && m_SharedQueue.async_Sync_Swap.empty() && m_SharedQueue.asyncQueue.empty();
	}

	bool AllTaskDone()
	{
		return AllAsyncTaskDone() && AllSyncTaskDone();
	}

	bool WaitAllAsyncTaskDone()
	{
		std::for_each(m_Threads.begin(), m_Threads.end(), std::mem_fun(&KWorkerThread::Wait));
		return true;
	}

	void ProcessSyncTask()
	{
		if(m_SharedQueue.bSwapNoEmpty)
		{
			{
				std::unique_lock<decltype(m_SharedQueue.lock)> lock(m_SharedQueue.lock);
				m_SharedQueue.async_Sync_Swap.swap(m_SharedQueue.syncQueue);
				m_SharedQueue.bSwapNoEmpty = false;
			}
			assert(!m_SharedQueue.syncQueue.empty());
			std::for_each(m_SharedQueue.syncQueue.begin(), m_SharedQueue.syncQueue.end(), [](Task& syncTask){syncTask();});
			m_SharedQueue.syncQueue.clear();
		}
	}
};

template<typename Task, bool bUseLockFreeQueue>
unsigned short KThreadPool<Task, bUseLockFreeQueue>::KWorkerThread::ms_IDCounter = 0;

template<typename Task, bool bUseLockFreeQueue>
std::mutex KThreadPool<Task, bUseLockFreeQueue>::KWorkerThread::ms_IDLock;

template<typename Task, bool bUseLockFreeQueue>
std::queue<unsigned short> KThreadPool<Task, bUseLockFreeQueue>::KWorkerThread::ms_IDQueue;