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

	IKAsyncTaskPtr CreateAsyncTask(IKTaskWorkPtr work) override;
	IKTaskThreadPool* GetThreadPool() override;

	void Init() override;
	void UnInit() override;
};