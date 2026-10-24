#pragma once
#include "Interface/Task/IKAsyncTask.h"
#include "Publish/KEvent.h"
#include <atomic>
#include <cstdint>

class KAsyncTask : public IKAsyncTask
{
protected:
	enum AsyncTaskState
	{
		ATS_IDLE = 0,
		ATS_QUEUED = 1,
		ATS_RUNNING = 2,
		ATS_DONE = -1,
	};

	IKTaskWorkPtr m_TaskWork;
	IKTaskThreadPool* m_ThreadPool;
	KEvent m_WorkFinishEvent;
	std::atomic<int32_t> m_State;

	void Execute();
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