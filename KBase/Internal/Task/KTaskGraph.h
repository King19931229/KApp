#pragma once
#include "Interface/Task/IKTaskGraph.h"
#include <vector>

class KGraphTask : public IKGraphTask
{
protected:
	IKGraphTaskEventRef m_TaskEvent;
	IKTaskWorkPtr m_TaskWork;
	IKGraphTaskQueue* m_TaskQueue;
	std::mutex m_WorkProcessLock;
public:
	KGraphTask(IKGraphTaskEventRef taskEvent, IKTaskWorkPtr taskWork, IKGraphTaskQueue* taskQueue);
	~KGraphTask();

	void SetTaskQueue(IKGraphTaskQueue* queue);
	void Abandon(IKGraphTaskRef thisTask);

	virtual void DoWork() override;	
	virtual const char* GetDebugInfo() override;
};

class KGraphTaskEvent : public IKGraphTaskEvent
{
protected:
	IKGraphTaskRef m_Task;

	std::mutex m_TaskProcessLock;
	std::shared_ptr<KEvent> m_DoneEvent;
	std::shared_ptr<bool> m_Done;
	bool m_Queued;
	bool m_AbandonOnClose;

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
	virtual bool AddSubsequent(IKGraphTaskEventRef subsequent) override;
	virtual bool SetThreadToExetuceOn(NamedThread::Type thread) override;
	virtual bool IsCompleted() const override;
	virtual void WaitForCompletion() override;
	virtual void Abandon() override;
	virtual const char* GetDebugInfo() override;

	void Dispatch();
	void OnTaskDone();
};