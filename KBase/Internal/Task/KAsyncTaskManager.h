#pragma once
#include "Interface/Task/IKAsyncTask.h"
#include "Internal/Task/KTaskThreadPool.h"

class KAsyncTaskManager : public IKAsyncTaskManager
{
protected:
	KTaskThreadPool m_ThreadPool;
public:
	KAsyncTaskManager();
	~KAsyncTaskManager();

	IKAsyncTaskRef CreateAsyncTask(IKTaskWorkPtr work) override;
	IKTaskThreadPool* GetThreadPool() override;

	void ParallelFor(std::function<void(uint32_t)> body, uint32_t taskNum) override;

	void Init() override;
	void UnInit() override;
};