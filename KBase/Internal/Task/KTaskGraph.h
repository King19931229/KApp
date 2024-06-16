#pragma once
#include "Interface/Task/IKTaskGraph.h"
#include <vector>

class KGraphTask : public IKGraphTask, std::enable_shared_from_this<KGraphTask>
{
protected:
	IKTaskWorkPtr m_TaskWork;
	IKGraphTaskQueue* m_TaskQueue;

	std::shared_ptr<KEvent> m_DoneEvent;
	std::shared_ptr<std::atomic_bool> m_Done;

	NamedThread::Type m_ThreadToExecuteOn;
	std::vector<IKGraphTask*> m_Subsequents;
	std::vector<IKGraphTask*> m_EventToWaitFor;
	std::atomic_int32_t m_NumPrerequisite;

	void SetupPrerequisite(const std::vector<IKGraphTaskPtr>& prerequisites);
	void DispatchSubsequents(bool abandon);
	void QueueTask();
	void ConditionalQueueTask(bool prerequisiteAbandon);
	void PrerequisiteComplete(int32_t num);
public:
	KGraphTask(IKTaskWorkPtr work, NamedThread::Type thread, const std::vector<IKGraphTaskPtr>& prerequisites, bool hold);
	KGraphTask(KGraphTask& stealFrom, const std::vector<IKGraphTaskPtr>& prerequisites, bool hold);
	~KGraphTask();

	virtual bool AddEventToWaitFor(IKGraphTask* eventToWait) override;
	virtual bool AddSubsequent(IKGraphTask* subsequent) override;
	virtual bool SetThreadToExetuceOn(NamedThread::Type thread) override;
	virtual bool IsCompleted() const override;
	virtual void WaitForCompletion() override;
	virtual void DoWork() override;
	virtual void Abandon() override;

	void Dispatch();	
};