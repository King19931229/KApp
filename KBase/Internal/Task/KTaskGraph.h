#pragma once
#include "Interface/Task/IKTaskGraph.h"
#include <vector>

class KGraphTask : public IKGraphTask, std::enable_shared_from_this<IKGraphTask>
{
protected:
	IKTaskWorkPtr m_TaskWork;
	IKGraphTaskQueue* m_TaskQueue;

	std::shared_ptr<KEvent> m_DoneEvent;
	std::shared_ptr<std::atomic_bool> m_Done;

	NamedThread m_ThreadToExecuteOn;
	std::vector<IKGraphTaskPtr> m_Subsequents;
	std::vector<IKGraphTaskPtr> m_EventToWaitFor;
	std::atomic_int32_t m_NumPrerequisite;

	void SetupPrerequisite(const std::vector<IKGraphTaskPtr>& prerequisites);
	void DispatchSubsequents(bool abandon);
	void QueueTask();
	void ConditionalQueueTask(bool prerequisiteAbandon);
	void PrerequisiteComplete(int32_t num);
public:
	KGraphTask(IKTaskWorkPtr work, NamedThread thread, const std::vector<IKGraphTaskPtr>& prerequisites, bool hold);
	KGraphTask(KGraphTask& stealFrom, const std::vector<IKGraphTaskPtr>& prerequisites, bool hold);
	~KGraphTask();

	virtual bool AddEventToWaitFor(IKGraphTaskPtr eventToWait) override;
	virtual bool AddSubsequent(IKGraphTaskPtr subsequent) override;
	virtual bool SetThreadToExetuceOn(NamedThread thread) override;
	virtual bool IsCompleted() const override;
	virtual void WaitForCompletion() override;
	virtual void DoWork() override;
	virtual void Abandon() override;

	void Dispatch();	
};

class KTaskGraphManager : public IKTaskGraphManager
{
public:
	KTaskGraphManager() {}
	~KTaskGraphManager() {}
};