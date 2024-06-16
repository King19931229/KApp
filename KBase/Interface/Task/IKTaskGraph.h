#pragma once
#include "KBase/Publish/KEvent.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Interface/Task/IKTaskWork.h"
#include <memory>
#include <vector>

enum TaskThreadPriority
{
	TASK_THREAD_PRIORITY_NORMAL,
	TASK_THREAD_PRIORITY_LOW,
	TASK_THREAD_PRIORITY_HIGH,
	TASK_THREAD_PRIORITY_NUM
};

enum TaskPriority
{
	TASK_PRIORITY_NORMAL,
	TASK_PRIORITY_LOW,
	TASK_PRIORITY_HIGH,
	TASK_PRIORITY_NUM
};

struct NamedThread
{
	typedef uint32_t Type;
	enum Enum
	{
		GAME_THREAD,
		RENDER_THREAD,
		RHI_THREAD,

		NUM_INTERNAL_THREAD,
		TASK_THREAD_START = NUM_INTERNAL_THREAD,

		ANY_THREAD = 0xFF,

		THREAD_PRIORITY_SHIFT = 8,
		THREAD_PRIORITY_MASK = 0X300,

		TASK_PRIORITY_SHIFT = 10,
		TASK_PRIORITY_MASK = 0XC00,

		ANY_THREAD_PRIORITY = (0 << THREAD_PRIORITY_SHIFT),
		LOW_THREAD_PRIORITY = ((1 + TASK_THREAD_PRIORITY_LOW) << THREAD_PRIORITY_SHIFT),
		NORMAL_THREAD_PRIORITY = ((1 + TASK_THREAD_PRIORITY_NORMAL) << THREAD_PRIORITY_SHIFT),
		HIGH_THREAD_PRIORITY = ((1 + TASK_THREAD_PRIORITY_HIGH) << THREAD_PRIORITY_SHIFT),

		LOW_TASK_PRIORITY = (TASK_PRIORITY_LOW << TASK_PRIORITY_SHIFT),
		NORMAL_TASK_PRIORITY = (TASK_PRIORITY_NORMAL << TASK_PRIORITY_SHIFT),
		HIGH_TASK_PRIORITY = (TASK_PRIORITY_HIGH << TASK_PRIORITY_SHIFT),

		ANY_THREAD_LOW_TASK_ANY_THREAD = ANY_THREAD | ANY_THREAD_PRIORITY | LOW_TASK_PRIORITY,
		ANY_THREAD_NORMAL_TASK_ANY_THREAD = ANY_THREAD | ANY_THREAD_PRIORITY | NORMAL_TASK_PRIORITY,
		ANY_THREAD_HIGH_TASK_ANY_THREAD = ANY_THREAD | ANY_THREAD_PRIORITY | HIGH_TASK_PRIORITY,

		LOW_THREAD_LOW_TASK_ANY_THREAD = ANY_THREAD | LOW_THREAD_PRIORITY | LOW_TASK_PRIORITY,
		LOW_THREAD_NORMAL_TASK_ANY_THREAD = ANY_THREAD | LOW_THREAD_PRIORITY | NORMAL_TASK_PRIORITY,
		LOW_THREAD_HIGH_TASK_ANY_THREAD = ANY_THREAD | LOW_THREAD_PRIORITY | HIGH_TASK_PRIORITY,

		NORMAL_THREAD_LOW_TASK_ANY_THREAD = ANY_THREAD | NORMAL_THREAD_PRIORITY | LOW_TASK_PRIORITY,
		NORMAL_THREAD_NORMAL_TASK_ANY_THREAD = ANY_THREAD | NORMAL_THREAD_PRIORITY | NORMAL_TASK_PRIORITY,
		NORMAL_THREAD_HIGH_TASK_ANY_THREAD = ANY_THREAD | NORMAL_THREAD_PRIORITY | HIGH_TASK_PRIORITY,

		HIGH_THREAD_LOW_TASK_ANY_THREAD = ANY_THREAD | HIGH_THREAD_PRIORITY | LOW_TASK_PRIORITY,
		HIGH_THREAD_NORMAL_TASK_ANY_THREAD = ANY_THREAD | HIGH_THREAD_PRIORITY | NORMAL_TASK_PRIORITY,
		HIGH_THREAD_HIGH_TASK_ANY_THREAD = ANY_THREAD | HIGH_THREAD_PRIORITY | HIGH_TASK_PRIORITY,
	};
};

struct IKTaskGraphWork
{
	virtual ~IKTaskGraphWork() {}
	virtual void DoWork() = 0;
	virtual void Abandon() {}
};
typedef std::shared_ptr<IKTaskGraphWork> IKTaskGraphWorkPtr;

struct IKGraphTask;
typedef std::shared_ptr<IKGraphTask> IKGraphTaskPtr;

struct IKGraphTask
{
	virtual ~IKGraphTask() {}
	virtual bool AddEventToWaitFor(IKGraphTask* eventToWait) = 0;
	virtual bool AddSubsequent(IKGraphTask* subsequent) = 0;
	virtual bool SetThreadToExetuceOn(NamedThread::Type thread) = 0;
	virtual bool IsCompleted() const = 0;
	virtual void WaitForCompletion() = 0;
	virtual void DoWork() = 0;
	virtual void Abandon() = 0;
};

struct IKGraphTaskQueue
{
	virtual ~IKGraphTaskQueue() {}
	virtual void Init() = 0;
	virtual void UnInit() = 0;
	virtual bool IsDone() const = 0;
	virtual void AddTask(IKGraphTask* task, TaskPriority priority) = 0;
	virtual void RemoveTask(IKGraphTask* task) = 0;
};

struct IKTaskGraphManager
{
	virtual ~IKTaskGraphManager() {}
	virtual void Init() = 0;
	virtual void UnInit() = 0;
	virtual IKGraphTaskPtr CreateAndDispatch(IKTaskWorkPtr task, NamedThread::Type thread, const std::vector<IKGraphTaskPtr>& prerequisites) = 0;
	virtual IKGraphTaskPtr CreateAndHold(IKTaskWorkPtr task, NamedThread::Type thread, const std::vector<IKGraphTaskPtr>& prerequisites) = 0;
	virtual void AddTask(IKGraphTask* task, NamedThread::Type thread) = 0;
	virtual void AttachToThread(NamedThread::Type thread) = 0;
	virtual void ProcessTaskUntilIdle(NamedThread::Type thread) = 0;
};

IKTaskGraphManager* GetTaskGraphManager();