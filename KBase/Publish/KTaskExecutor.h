#pragma once
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KSpinLock.h"
#include <memory>
#include <assert.h>

enum TaskState
{
	// 该任务已经提交到多线程命令队列里
	TS_PENDING_ASYNC,
	// 工作线程正在执行该任务异步部分
	TS_LOADING_ASYNC,
	// 工作线程已经完成该任务异步部分 但是同步任务还没被执行
	TS_LOADED_ASYNC, /* TS_PENDING_SYNC */
	// 该任务的同步部分已经进入到队列中 等待主线程轮训
	TS_LOADING_SYNC,
	// 该任务被执行完全执行完毕
	TS_LOADED,
	// 该任务执行失败
	TS_LOAD_FAIL /*TS_CANCELED */
};

struct IKTaskUnit
{
	virtual ~IKTaskUnit() {}
	virtual bool AsyncLoad() = 0;
	virtual bool SyncLoad() = 0;
	virtual bool HasSyncLoad() const = 0;
};
typedef std::shared_ptr<IKTaskUnit> KTaskUnitPtr;

class KSampleAsyncTaskUnit : public IKTaskUnit
{
public:
	typedef std::function<bool()> FuncType;
protected:
	FuncType m_AsyncFunc;
public:
	KSampleAsyncTaskUnit(FuncType asyncFunc)
		: m_AsyncFunc(asyncFunc)
	{}
	virtual bool AsyncLoad() { return m_AsyncFunc(); }
	virtual bool SyncLoad() { return false;	}
	virtual bool HasSyncLoad() const { return false; }
};

class KSampleTaskUnit : public IKTaskUnit
{
public:
	typedef std::function<bool()> FuncType;
protected:
	FuncType m_AsyncFunc;
	FuncType m_SyncFunc;
public:
	KSampleTaskUnit(FuncType asyncFunc, FuncType syncFunc)
		: m_AsyncFunc(asyncFunc),
		m_SyncFunc(syncFunc)
	{}
	virtual bool AsyncLoad() { return m_AsyncFunc(); }
	virtual bool SyncLoad() { return m_SyncFunc(); }
	virtual bool HasSyncLoad() const { return true; }
};

template<bool>
class KTaskExecutor;

typedef std::shared_ptr<KSemaphore> KSemaphorePtr;

struct KTaskUnitProcessor
{
	friend class KTaskExecutor<true>;
	friend class KTaskExecutor<false>;
	friend class KTaskUnitProcessorGroup;
protected:
	std::mutex m_WaitLock;
	std::atomic_uchar m_eState;
	KSemaphorePtr m_pSem;
	KTaskUnitPtr m_pTaskUnit;

	KTaskUnitProcessor(const KTaskUnitProcessor& rhs) = delete;
	KTaskUnitProcessor& operator=(const KTaskUnitProcessor& rhs) = delete;
	KTaskUnitProcessor(KTaskUnitProcessor&& rhs) = delete;
	KTaskUnitProcessor& operator=(KTaskUnitProcessor&& rhs) = delete;

	inline bool NotifySem() { return m_pSem->Notify(); }
	inline bool WaitSem() { return m_pSem->Wait(); }
	inline bool AsyncLoad() { return m_pTaskUnit ? m_pTaskUnit->AsyncLoad() : false; }
	inline bool SyncLoad() { return m_pTaskUnit? m_pTaskUnit->SyncLoad() : false; }
	inline bool HasSyncLoad() const { return m_pTaskUnit ? m_pTaskUnit->HasSyncLoad() : false; }
	inline bool IsDone() const { return m_eState.load() >= TS_LOADED; }
public:
	explicit KTaskUnitProcessor(KTaskUnitPtr pUnit)
		: m_pTaskUnit(pUnit)
	{
		m_pSem = KSemaphorePtr(KNEW KSemaphore);
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
			if(m_eState.compare_exchange_weak(uExp, TS_LOAD_FAIL))
				break;
		}
		// 这里就是 TS_LOAD_FAIL
		assert(m_eState.load() == TS_LOAD_FAIL);
		return true;
	}

	/*
	*@brief 调用线程挂起等待直到任务完成
	*/
	bool Wait()
	{
		std::lock_guard<decltype(m_WaitLock)> lockGuard(m_WaitLock);
		if (IsDone())
		{
			return true;
		}
		WaitSem();
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
		m_pSem = KSemaphorePtr(KNEW KSemaphore);
	}
	bool Cancel()
	{
		for(TaskList::iterator it = m_TaskList.begin(); it != m_TaskList.end(); ++it)
		{
			KTaskUnitProcessorPtr pUnit = *it;
			pUnit->Cancel();
		}
		m_TaskList.clear();
		m_pSem = KSemaphorePtr(KNEW KSemaphore);
		return true;
	}
	bool Wait()
	{
		for(TaskList::iterator it = m_TaskList.begin(); it != m_TaskList.end(); ++it)
		{
			KTaskUnitProcessorPtr pUnit = *it;
			pUnit->WaitSem();
		}
		m_TaskList.clear();
		m_pSem = KSemaphorePtr(KNEW KSemaphore);
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
						if(!pTask->HasSyncLoad())
						{
							pTask->NotifySem();
						}
						return true;
					}
				}
				else
				{
					if(pTask->m_eState.compare_exchange_strong(uExp, TS_LOADED))
					{
						if(!pTask->HasSyncLoad())
						{
							pTask->NotifySem();
						}
						return true;
					}
				}
			}
		}
		while(true)
		{
			uExp = pTask->m_eState.load();
			if(pTask->m_eState.compare_exchange_weak(uExp, TS_LOAD_FAIL))
				break;
			else
				std::this_thread::yield();
		}
		if(!pTask->HasSyncLoad())
		{
			pTask->NotifySem();
		}
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
				{
					pTask->NotifySem();
					return true;
				}
			}
		}
		while(true)
		{
			uExp = pTask->m_eState.load();
			if(pTask->m_eState.compare_exchange_weak(uExp, TS_LOAD_FAIL))
				break;
			else
				std::this_thread::yield();
		}
		pTask->NotifySem();
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
			pTask = KTaskUnitProcessorPtr(KNEW KTaskUnitProcessor(pUnit));
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
		KTaskUnitProcessorGroupPtr pGroup = KTaskUnitProcessorGroupPtr(KNEW KTaskUnitProcessorGroup());
		return pGroup;
	}

	KTaskUnitProcessorPtr Submit(KTaskUnitProcessorGroupPtr pGroup, KTaskUnitPtr pUnit)
	{
		KTaskUnitProcessorPtr pTask = nullptr;
		if(pGroup && pUnit)
		{
			if(pUnit)
				pTask = KTaskUnitProcessorPtr(KNEW KTaskUnitProcessor(pUnit, pGroup->GetSemaphore()));

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
	inline void WaitAllAsyncTaskDone() { m_ExecutePool.WaitAllAsyncTaskDone(); }
	inline void Init(const std::string& workThreadName, size_t uThreadNum) { m_ExecutePool.Init(workThreadName, uThreadNum); }
	inline void UnInit() { m_ExecutePool.UnInit(); }
	inline size_t GetWorkerThreadNum() { return m_ExecutePool.GetWorkerThreadNum(); }
};