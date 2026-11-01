#pragma once
#include "Interface/Task/IKTaskGraph.h"
#include <atomic>
#include <cstdint>
#include <vector>

class KGraphTask : public IKGraphTask
{
friend class KGraphTaskEvent;
protected:
	IKGraphTaskEventRef m_TaskEvent;
	IKTaskWorkPtr m_TaskWork;
	std::atomic<IKGraphTaskQueue*> m_TaskQueue;

	void Abandon(IKGraphTaskRef thisTask);
public:
	KGraphTask(IKGraphTaskEventRef taskEvent, IKTaskWorkPtr taskWork, IKGraphTaskQueue* taskQueue);
	~KGraphTask();

	void SetTaskQueue(IKGraphTaskQueue* queue);

	virtual void DoWork() override;	
	virtual void DetachFromEvent() override;
	virtual const char* GetDebugInfo() override;
};

class KGraphTaskEvent : public IKGraphTaskEvent
{
friend class KGraphTask;
protected:
	enum GraphTaskState
	{
		GTS_IDLE = 0,
		GTS_RUNNING = 1,
		GTS_DONE = -1,
	};

	IKGraphTaskRef m_Task;

	std::mutex m_TaskProcessLock;
	std::shared_ptr<KEvent> m_DoneEvent;
	std::shared_ptr<std::atomic<int32_t>> m_State;
	bool m_Queued;

	NamedThread::Type m_ThreadToExecuteOn;
	std::vector<IKGraphTaskEventRef> m_Subsequents;
	std::vector<IKGraphTaskEventRef> m_EventToWaitFor;
	std::atomic_int32_t m_NumPrerequisite;

	static void SetupPrerequisite(IKGraphTaskEventRef taskEvent, const std::vector<IKGraphTaskEventRef>& prerequisites);
	void DispatchSubsequents(bool abandon);
	void QueueTask();
	void ConditionalQueueTask(bool prerequisiteAbandon);
	void PrerequisiteComplete(int32_t num);
public:
	KGraphTaskEvent(NamedThread::Type thread);
	KGraphTaskEvent(KGraphTaskEvent& stealFrom);
	~KGraphTaskEvent();

	static void Setup(IKGraphTaskEventRef taskEvent, IKTaskWorkPtr work, const std::vector<IKGraphTaskEventRef>& prerequisites, bool hold);
	static void Setup(IKGraphTaskEventRef taskEvent, const std::vector<IKGraphTaskEventRef>& prerequisites, bool hold);

	virtual bool AddEventToWaitFor(IKGraphTaskEventRef eventToWait) override;
	virtual bool SetThreadToExetuceOn(NamedThread::Type thread) override;
	virtual bool IsCompleted() const override;
	virtual void WaitForCompletion() override;
	virtual void Abandon() override;
	virtual const char* GetDebugInfo() override;

	bool AddSubsequent(IKGraphTaskEventRef subsequent);
	void Dispatch();
	void OnTaskDone();
};