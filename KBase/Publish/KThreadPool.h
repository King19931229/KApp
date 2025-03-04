#pragma once
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KThreadTool.h"
#include <list>
#include <assert.h>

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

	template<bool __UseLockFreeQueue__>
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
	typedef std::list<Task> SharedQueueType;

	struct TripleQueue
	{
		SharedQueueType asyncQueue;
		SharedQueueType asyncSyncSwapQueue;
		SharedQueueType syncQueue;
		std::mutex		lock;
		std::condition_variable cond;

		TripleQueue()
		{

		}
	};

	////////////////////////////////////////////////////////////
	class KWorkerThread
	{
	protected:
		QueueType& m_Queue;
		TripleQueue& m_SharedQueue;

		KSemaphore& m_Sem;

		std::mutex& m_WaitMutex;
		std::condition_variable& m_WaitCond;

		const std::string& m_ThreadName;

		const bool& m_bDone;
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
				snprintf(szBuffer, sizeof(szBuffer), "%s %d", m_ThreadName.c_str(), uID);
#else
				sprintf_s(szBuffer, sizeof(szBuffer), "%s %d", m_ThreadName.c_str(), uID);
#endif
				KThreadTool::SetThreadName(szBuffer);
			}

			TaskGroup taskGroup;
			while(!m_bDone)
			{
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
						m_SharedQueue.cond.notify_one();
					}
				});

				if(m_Queue.Empty())
				{
					std::lock_guard<std::mutex> lock(m_WaitMutex);
					m_WaitCond.notify_all();
				}
			}

			{
				std::lock_guard<decltype(ms_IDLock)> lockGuard(ms_IDLock);
				ms_IDQueue.push(uID);
			}
		}
	public:
		KWorkerThread(QueueType& queue,
			TripleQueue& sharedQueue,
			KSemaphore& sem,
			std::mutex& waitMutex,
			std::condition_variable& waitCond,
			const std::string& threadName,
			bool& done)
			: m_Queue(queue)
			, m_SharedQueue(sharedQueue)
			, m_Sem(sem)
			, m_WaitMutex(waitMutex)
			, m_WaitCond(waitCond)
			, m_ThreadName(threadName)
			, m_bDone(done)
			, m_Thread(&KWorkerThread::ThreadFunc, this)
		{
		}

		~KWorkerThread()
		{
			assert(m_bDone);
			m_Sem.NotifyAll();
			m_Thread.join();
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
			KThreadTool::SetThreadName("SyncTaskSwapThread");
			while (!m_bDone)
			{
				std::unique_lock<decltype(m_SharedQueue.lock)> lock(m_SharedQueue.lock);
				m_SharedQueue.cond.wait(lock, [this] { return !m_SharedQueue.asyncQueue.empty() || m_bDone; });

				if (m_bDone)
				{
					break;
				}

				if (m_SharedQueue.asyncSyncSwapQueue.empty())
				{
					if (!m_SharedQueue.asyncQueue.empty())
					{
						m_SharedQueue.asyncSyncSwapQueue.swap(m_SharedQueue.asyncQueue);
					}
				}
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
			m_SharedQueue.cond.notify_one();
			m_Thread.join();
		}

		bool Notify()
		{
			return m_SharedQueue.sem.Notify();
		}
	};
	////////////////////////////////////////////////////////////
protected:
	QueueType m_Queue;
	TripleQueue m_SharedQueue;

	KSemaphore m_Sem;
	std::mutex m_WaitMutex;
	std::condition_variable m_WaitCond;
	bool m_bDone;

	std::string m_WorkerThreadName;

	std::vector<KWorkerThread*> m_Threads;
	KSyncTaskThread *m_pSyncTaskThread;
public:
	KThreadPool()
		: m_pSyncTaskThread(nullptr)
		, m_bDone(true)
		, m_WorkerThreadName("WorkThread")
	{
	}

	~KThreadPool()
	{
		assert(m_Threads.empty());
		assert(m_pSyncTaskThread == nullptr);
#if _DEBUG
		assert(m_Sem.GetCount() == 0);
#endif
	}

	void Init(const std::string& workThreadName, size_t uThreadNum)
	{
		assert(!m_pSyncTaskThread);
		m_WorkerThreadName = workThreadName;
		m_bDone = false;
		if (!m_pSyncTaskThread)
		{
			m_pSyncTaskThread = KNEW KSyncTaskThread(m_SharedQueue);
			while (uThreadNum--)
			{
				KWorkerThread* pThread = KNEW KWorkerThread(m_Queue, m_SharedQueue, m_Sem, m_WaitMutex, m_WaitCond, m_WorkerThreadName, m_bDone);
				assert(pThread);
				m_Threads.push_back(pThread);
			}
		}
	}

	void UnInit()
	{
		m_bDone = true;
		if (m_pSyncTaskThread)
		{
			KDELETE m_pSyncTaskThread;
			m_pSyncTaskThread = nullptr;
		}
		for (auto it = m_Threads.begin(), itEnd = m_Threads.end();
			it != itEnd; ++it)
		{
			delete *it;
			*it = nullptr;
		}
		m_Threads.clear();
		m_Sem.Reset();
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
		m_Sem.Notify();
	}

	void SubmitTask(Task asyncTask, Task syncTask)
	{
		TaskGroup group;

		group.asyncTask = asyncTask;
		group.syncTask = syncTask;
		group.bHasSyncTask = true;

		m_Queue.Push(group);
		m_Sem.Notify();
	}

	bool AllAsyncTaskDone()
	{
		return m_Queue.Empty();
	}

	bool AllSyncTaskDone()
	{
		std::unique_lock<decltype(m_SharedQueue.lock)> lock(m_SharedQueue.lock);
		return m_SharedQueue.syncQueue.empty() && m_SharedQueue.asyncSyncSwapQueue.empty() && m_SharedQueue.asyncQueue.empty();
	}

	bool AllTaskDone()
	{
		return AllAsyncTaskDone() && AllSyncTaskDone();
	}

	bool WaitAllAsyncTaskDone()
	{
		std::unique_lock<std::mutex> lock(m_WaitMutex);
		m_WaitCond.wait(lock, [this](){ return m_Queue.Empty(); });
		return true;
	}

	void ProcessSyncTask()
	{
		if (!m_SharedQueue.syncQueue.empty())
		{
			std::for_each(m_SharedQueue.syncQueue.begin(), m_SharedQueue.syncQueue.end(), [](Task& syncTask) {syncTask(); });
			m_SharedQueue.syncQueue.clear();
		}
		std::unique_lock<decltype(m_SharedQueue.lock)> lock(m_SharedQueue.lock);
		if (!m_SharedQueue.asyncSyncSwapQueue.empty())
		{
			m_SharedQueue.asyncSyncSwapQueue.swap(m_SharedQueue.syncQueue);
		}
	}
};

template<typename Task, bool bUseLockFreeQueue>
unsigned short KThreadPool<Task, bUseLockFreeQueue>::KWorkerThread::ms_IDCounter = 0;

template<typename Task, bool bUseLockFreeQueue>
std::mutex KThreadPool<Task, bUseLockFreeQueue>::KWorkerThread::ms_IDLock;

template<typename Task, bool bUseLockFreeQueue>
std::queue<unsigned short> KThreadPool<Task, bUseLockFreeQueue>::KWorkerThread::ms_IDQueue;