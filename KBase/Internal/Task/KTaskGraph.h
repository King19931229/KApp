#pragma once
#include "Interface/Task/IKTaskGraph.h"
#include <vector>

class KGraphTask : public IKGraphTask
{
protected:
	IKTaskWorkPtr m_TaskWork;
	IKGraphTaskQueue* m_TaskQueue;

	std::mutex m_TaskProcessLock;
	std::shared_ptr<KEvent> m_DoneEvent;
	bool m_Done;

	NamedThread::Type m_ThreadToExecuteOn;
	std::vector<IKGraphTask*> m_Subsequents;
	std::vector<IKGraphTaskRef> m_EventToWaitFor;
	std::atomic_int32_t m_NumPrerequisite;

	void SetupPrerequisite(const std::vector<IKGraphTaskRef>& prerequisites);
	void DispatchSubsequents(bool abandon);
	void QueueTask();
	void ConditionalQueueTask(bool prerequisiteAbandon);
	void PrerequisiteComplete(int32_t num);
public:
	KGraphTask(IKTaskWorkPtr work, NamedThread::Type thread);
	KGraphTask(KGraphTask& stealFrom);
	~KGraphTask();

	static void Setup(IKGraphTaskRef task, const std::vector<IKGraphTaskRef>& prerequisites, bool hold);

	virtual bool AddEventToWaitFor(IKGraphTaskRef eventToWait) override;
	virtual bool AddSubsequent(IKGraphTask* subsequent) override;
	virtual bool SetThreadToExetuceOn(NamedThread::Type thread) override;
	virtual bool IsCompleted() const override;
	virtual void WaitForCompletion() override;
	virtual void DoWork() override;
	virtual void Abandon() override;
	virtual const char* GetDebugInfo() override;

	void Dispatch();	
};