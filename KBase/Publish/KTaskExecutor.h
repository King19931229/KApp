#pragma once
#include "Publish/KThreadPool.h"
#include <memory>
#include <assert.h>

enum TaskState
{
	TS_PENDING_ASYNC,

	TS_LOADING_ASYNC,

	TS_LOADED_ASYNC,/* TS_PENDING_SYNC */

	TS_LOADING_SYNC,
	TS_LOADED,

	TS_CANCELING,
	TS_LOAD_FAIL
};

struct IKTaskUnit
{
	virtual ~IKTaskUnit() {}
	virtual bool AsyncLoad() = 0;
	virtual bool SyncLoad() = 0;
	virtual bool HasSyncLoad() const = 0;
};
typedef std::shared_ptr<IKTaskUnit> KTaskUnitPtr;

template<bool>
class KTaskExecutor;

typedef std::shared_ptr<KSemaphore> KSemaphorePtr;

struct KTaskUnitProcessor
{
	friend class KTaskExecutor<true>;
	friend class KTaskExecutor<false>;
	friend class KTaskUnitProcessorGroup;
protected:
	std::atomic_uchar m_eState;
	KSemaphorePtr m_pSem;
	KTaskUnitPtr m_pTaskUnit;

	KTaskUnitProcessor(const KTaskUnitProcessor& rhs){}
	KTaskUnitProcessor& operator=(const KTaskUnitProcessor& rhs){}

	inline bool Notify() { return m_pSem->Notify(); }
	inline bool Wait() { return m_pSem->Wait(); }
	inline void Reset() { m_eState.store(TS_PENDING_ASYNC); }
	inline bool AsyncLoad() { return m_pTaskUnit ? m_pTaskUnit->AsyncLoad() : false; }
	inline bool SyncLoad() { return m_pTaskUnit? m_pTaskUnit->SyncLoad() : false; }
	inline bool HasSyncLoad() const { return m_pTaskUnit ? m_pTaskUnit->HasSyncLoad() : false; }
	inline bool IsDone() const { return m_eState.load() >= TS_LOADED; }
public:
	explicit KTaskUnitProcessor(KTaskUnitPtr pUnit)
		: m_pTaskUnit(pUnit)
	{
		m_pSem = KSemaphorePtr(new KSemaphore);
		m_eState.store(TS_PENDING_ASYNC);
	}
	explicit KTaskUnitProcessor(KTaskUnitPtr pUnit, KSemaphorePtr pSem)
		: m_pTaskUnit(pUnit)
	{
		m_pSem = pSem;
		m_eState.store(TS_PENDING_ASYNC);
	}
	~KTaskUnitProcessor()
	{
	}

	/*
	*@brief 获取当前线程任务状态
	*/
	inline TaskState GetState() const { return (TaskState)m_eState.load(); }

	/*
	*@brief 调用线程取消异步任务
	*@note 如果本单元任务有SyncLoad且取消时本单元异步任务已经完成则同步任务队列会保留本单元引用
	*/
	bool Cancel()
	{
		while(true)
		{
			unsigned char uExp = m_eState.load();
			// 加载已经失败了
			if(uExp == TS_LOAD_FAIL)
				return true;
			if(m_eState.compare_exchange_strong(uExp, TS_CANCELING))
				break;
		}
		// 这里不是 TS_CANCELING 就是 TS_LOAD_FAIL
		assert(m_eState.load() == TS_CANCELING || m_eState.load() == TS_LOAD_FAIL);
		return true;
	}

	/*
	*@brief 调用线程挂起等待直到异步任务完成
	*/
	bool WaitAsync()
	{
		unsigned char uExp = 0;

		uExp = TS_PENDING_ASYNC;
		if(m_eState.compare_exchange_strong(uExp, TS_PENDING_ASYNC))
		{
			Wait();
			return true;
		}

		uExp = TS_LOADING_ASYNC;
		if(m_eState.compare_exchange_strong(uExp, TS_LOADING_ASYNC))
		{
			Wait();
			return true;
		}

		assert(IsDone());
		return true;
	}
};
typedef std::shared_ptr<KTaskUnitProcessor> KTaskUnitProcessorPtr;

class KTaskUnitProcessorGroup
{
	friend class KTaskExecutor<true>;
	friend class KTaskExecutor<false>;
protected:
	KSemaphorePtr m_pSem;
	typedef std::vector<KTaskUnitProcessorPtr> TaskList;
	TaskList m_TaskList;

	inline void AddIntoList(KTaskUnitProcessorPtr pUnit) { m_TaskList.push_back(pUnit); }
	inline KSemaphorePtr GetSemaphore() { return m_pSem; }
public:
	KTaskUnitProcessorGroup()
	{
		m_pSem = KSemaphorePtr(new KSemaphore);
	}
	bool Cancel()
	{
		for(TaskList::iterator it = m_TaskList.begin(); it != m_TaskList.end(); ++it)
		{
			KTaskUnitProcessorPtr pUnit = *it;
			pUnit->Cancel();
		}
		m_TaskList.clear();
		m_pSem = KSemaphorePtr(new KSemaphore);
		return true;
	}
	bool WaitAsync()
	{
		for(TaskList::iterator it = m_TaskList.begin(); it != m_TaskList.end(); ++it)
		{
			KTaskUnitProcessorPtr pUnit = *it;
			pUnit->Wait();
		}
		m_TaskList.clear();
		m_pSem = KSemaphorePtr(new KSemaphore);
		return true;
	}
};
typedef std::shared_ptr<KTaskUnitProcessorGroup> KTaskUnitProcessorGroupPtr;

template<bool bUseLockFreeQueue = true>
class KTaskExecutor
{
protected:
	KThreadPool<std::function<bool()>, bUseLockFreeQueue> m_ExecutePool;
public:
	static bool AsyncFunc(KTaskUnitProcessorPtr pTask)
	{
		unsigned char uExp = TS_PENDING_ASYNC;
		if(pTask->m_eState.compare_exchange_strong(uExp, TS_LOADING_ASYNC))
		{
			if(pTask->AsyncLoad())
			{
				uExp = TS_LOADING_ASYNC;
				if(pTask->HasSyncLoad())
				{
					if(pTask->m_eState.compare_exchange_strong(uExp, TS_LOADED_ASYNC))
					{
						pTask->Notify();
						return true;
					}
				}
				else
				{
					if(pTask->m_eState.compare_exchange_strong(uExp, TS_LOADED))
					{
						pTask->Notify();
						return true;
					}
				}
			}
		}
		while(true)
		{
			uExp = pTask->m_eState.load();
			if(pTask->m_eState.compare_exchange_strong(uExp, TS_LOAD_FAIL))
				break;
			else
				std::this_thread::yield();
		}
		pTask->Notify();
		return false;
	}

	static bool SyncFunc(KTaskUnitProcessorPtr pTask)
	{
		unsigned char uExp = TS_LOADED_ASYNC;
		if(pTask->m_eState.compare_exchange_strong(uExp, TS_LOADING_SYNC))
		{
			if(pTask->SyncLoad())
			{
				uExp = TS_LOADING_SYNC;
				if(pTask->m_eState.compare_exchange_strong(uExp, TS_LOADED))
					return true;
			}
		}
		while(true)
		{
			uExp = pTask->m_eState.load();
			if(pTask->m_eState.compare_exchange_strong(uExp, TS_LOAD_FAIL))
				break;
			else
				std::this_thread::yield();
		}
		return false;
	}

	KTaskExecutor(const KTaskExecutor& rhs){}
	KTaskExecutor& operator=(const KTaskExecutor& rhs){}
public:
	KTaskExecutor()
	{
	}

	~KTaskExecutor()
	{
	}

	KTaskUnitProcessorPtr Submit(KTaskUnitPtr pUnit)
	{
		KTaskUnitProcessorPtr pTask = nullptr;
		if(pUnit)
		{
			pTask = KTaskUnitProcessorPtr(new KTaskUnitProcessor(pUnit));
			if(pTask->HasSyncLoad())
				m_ExecutePool.SubmitTask(
				[this, pTask]() -> bool { return AsyncFunc(pTask); },
				[this, pTask]() -> bool { return SyncFunc(pTask); });
			else
				m_ExecutePool.SubmitTask(
				[this, pTask]() -> bool { return AsyncFunc(pTask); });
		}
		return pTask;
	}

	KTaskUnitProcessorGroupPtr CreateGroup()
	{
		KTaskUnitProcessorGroupPtr pGroup = nullptr;
		pGroup = KTaskUnitProcessorGroupPtr(new KTaskUnitProcessorGroup());
		assert(pGroup);
		return pGroup;
	}

	KTaskUnitProcessorPtr Submit(KTaskUnitProcessorGroupPtr pGroup, KTaskUnitPtr pUnit)
	{
		KTaskUnitProcessorPtr pTask = nullptr;
		if(pGroup && pUnit)
		{
			if(pUnit)
				pTask = KTaskUnitProcessorPtr(new KTaskUnitProcessor(pUnit, pGroup->GetSemaphore()));

			if(pTask->HasSyncLoad())
				m_ExecutePool.SubmitTask(
				[this, pTask]() -> bool { return AsyncFunc(pTask); },
				[this, pTask]() -> bool { return SyncFunc(pTask); });
			else
				m_ExecutePool.SubmitTask(
				[this, pTask]() -> bool { return AsyncFunc(pTask); });

			pGroup->AddIntoList(pTask);
		}
		return pTask;
	}

	inline bool AllAsyncTaskDone() { return m_ExecutePool.AllAsyncTaskDone(); }
	inline bool AllSyncTaskDone() { return m_ExecutePool.AllSyncTaskDone(); }
	inline bool AllTaskDone() { return m_ExecutePool.AllTaskDone(); }
	inline void ProcessSyncTask() { m_ExecutePool.ProcessSyncTask(); }
	inline void PushWorkerThreads(size_t uThreadNum) { m_ExecutePool.PushWorkerThreads(uThreadNum); }
	inline void PopWorkerThreads(size_t uThreadNum) { m_ExecutePool.PopWorkerThreads(uThreadNum); }
	inline size_t GetWorkerThreadNum() { return m_ExecutePool.GetWorkerThreadNum(); }
};