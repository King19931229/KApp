#pragma once
#include "KThreadPool.h"
#include <memory>

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

struct KTaskUnit
{
	virtual ~KTaskUnit() {}
	// User Interface
	virtual bool AsyncLoad() = 0;
	virtual bool SyncLoad() = 0;
	virtual bool HasSyncLoad() const = 0;
};
typedef std::shared_ptr<KTaskUnit> KTaskUnitPtr;

struct KTaskUnitProcessor
{
	friend class KTaskExecutor;
protected:
	std::atomic_uchar m_eState;
	KSemaphore m_Sem;
	KTaskUnitPtr m_pTaskUnit;
	KTaskUnitProcessor(const KTaskUnitProcessor& rhs){}
	KTaskUnitProcessor& operator=(const KTaskUnitProcessor& rhs){}
	inline bool Notify() { return m_Sem.Notify(); }
	inline bool Wait() { return m_Sem.Wait(); }
	void Reset()
	{
		m_eState = TS_PENDING_ASYNC;
	}
public:
	KTaskUnitProcessor(KTaskUnitPtr pUnit)
		: m_pTaskUnit(pUnit)
	{
		m_eState.store(TS_PENDING_ASYNC);
	}
	~KTaskUnitProcessor()
	{
	}
	inline TaskState GetState() const { return (TaskState)m_eState.load();	}
	inline bool AsyncLoad() { return m_pTaskUnit->AsyncLoad(); }
	inline bool SyncLoad() { return m_pTaskUnit->SyncLoad(); }
	inline bool HasSyncLoad() const { return m_pTaskUnit->HasSyncLoad(); }

	bool Cancel(bool bWait)
	{
		while(true)
		{
			unsigned char uExp = m_eState.load();
			if(uExp == TS_LOAD_FAIL)
				return true;
			if(m_eState.compare_exchange_strong(uExp, TS_CANCELING))
				break;
		}
		// 这里不是 TS_CANCELING 就是 TS_LOAD_FAIL
		assert(m_eState.load() == TS_CANCELING || m_eState.load() == TS_LOAD_FAIL);
		if(bWait)
		{
			unsigned char uExp = TS_CANCELING;
			if(m_eState.compare_exchange_strong(uExp, TS_LOAD_FAIL))
				Wait();
		}
		return true;
	}
};
typedef std::shared_ptr<KTaskUnitProcessor> KTaskUnitProcessorPtr;

class KTaskExecutor
{
protected:
	KThreadPool<std::function<bool()>> m_ExecutePool;
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

	void Submit(KTaskUnitProcessorPtr pTask)
	{
		if(pTask->HasSyncLoad())
			m_ExecutePool.SubmitTask(
			std::bind(&KTaskExecutor::AsyncFunc, pTask),
			std::bind(&KTaskExecutor::SyncFunc, pTask));
		else
			m_ExecutePool.SubmitTask(std::bind(&KTaskExecutor::AsyncFunc, pTask));
	}

	inline bool AllTaskDone() { return m_ExecutePool.AllTaskDone(); }
	inline void ProcessSyncTask() { m_ExecutePool.ProcessSyncTask(); }
	inline void PushWorkerThreads(size_t uThreadNum) { m_ExecutePool.PushWorkerThreads(uThreadNum); }
	inline void PopWorkerThreads(size_t uThreadNum) { m_ExecutePool.PopWorkerThreads(uThreadNum); }
	inline size_t GetWorkerThreadNum() { return m_ExecutePool.GetWorkerThreadNum(); }
};