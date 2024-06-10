#pragma once
#include "KBase/Publish/KEvent.h"
#include "KBase/Interface/Task/IKTaskWork.h"
#include <memory>
#include <vector>

enum NamedThread
{
	GAME_THREAD,
	RENDER_THREAD,
	RHI_THREAD,
	NUM_INTERNAL_THREAD,
	ANY_THREAD = 0xFF
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
	virtual bool AddEventToWaitFor(IKGraphTaskPtr eventToWait) = 0;
	virtual bool AddSubsequent(IKGraphTaskPtr subsequent) = 0;
	virtual bool SetThreadToExetuceOn(NamedThread thread) = 0;
	virtual bool IsCompleted() const = 0;
	virtual void WaitForCompletion() = 0;
	virtual void DoWork() = 0;
	virtual void Abandon() = 0;
};

enum TaskThreadPriority
{
	TASK_THREAD_PRIORITY_LOW,
	TASK_THREAD_PRIORITY_MEDIUM,
	TASK_THREAD_PRIORITY_HIGH
};

struct IKGraphTaskQueue
{
	virtual ~IKGraphTaskQueue() {}
	virtual void StartUp(TaskThreadPriority threadPriority, uint32_t numTaskPriority, uint32_t threadNum) = 0;
	virtual void ShutDown() = 0;
	virtual void PushTask(IKGraphTask* task) = 0;
	virtual void PopTask(IKGraphTask* task) = 0;
};

struct IKTaskGraphManager
{
	virtual void Init() = 0;
	virtual void UnInit() = 0;
	virtual IKGraphTaskPtr CreateAndDispatch(IKTaskWorkPtr task, NamedThread thread, const std::vector<IKGraphTaskPtr>& prerequisites) = 0;
	virtual IKGraphTaskPtr CreateAndHold(IKTaskWorkPtr task, NamedThread thread, const std::vector<IKGraphTaskPtr>& prerequisites) = 0;
	virtual IKGraphTaskQueue* GetQueue(NamedThread thread) = 0;
	virtual void AttachToThread(NamedThread thread) = 0;
};

IKTaskGraphManager* GetTaskGraphManager();