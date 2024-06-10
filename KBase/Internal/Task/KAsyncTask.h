#pragma once
#include "Interface/Task/IKAsyncTask.h"
#include "Publish/KEvent.h"

class KAsyncTask : public IKAsyncTask
{
protected:
	IKTaskWorkPtr m_TaskWork;
	IKTaskThreadPool* m_ThreadPool;
	KEvent m_WorkFinishEvent;
	std::atomic<int32_t> m_WorkCounter;
public:
	KAsyncTask(IKTaskWorkPtr task);
	~KAsyncTask();

	virtual bool IsCompleted() const override;
	virtual void WaitForCompletion() override;
	virtual void StartAsync() override;
	virtual void StartSync() override;

	virtual void DoWork() override;
	virtual void Abandon() override;
};