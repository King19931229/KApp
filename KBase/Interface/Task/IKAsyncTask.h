#pragma once
#include "IKTaskWork.h"

struct IKTaskThreadPoolWork
{
	virtual ~IKTaskThreadPoolWork() {}
	virtual void DoWork() = 0;
	virtual void Abandon() = 0;
};

struct IKTaskThreadPool
{
	virtual ~IKTaskThreadPool() {}
	virtual void StartUp(uint32_t threadNum) = 0;
	virtual void ShutDown() = 0;
	virtual void PushTask(IKTaskThreadPoolWork* task) = 0;
	virtual void PopTask(IKTaskThreadPoolWork* task) = 0;
};

struct IKAsyncTask : public IKTaskThreadPoolWork
{
	virtual ~IKAsyncTask() {}
	virtual bool IsCompleted() const = 0;
	virtual void WaitForCompletion() = 0;
	virtual void StartAsync() = 0;
	virtual void StartSync() = 0;
};
typedef std::shared_ptr<IKAsyncTask> IKAsyncTaskPtr;

struct IKAsyncTaskManager
{
	virtual ~IKAsyncTaskManager() {}
	virtual IKAsyncTaskPtr CreateAsyncTask(IKTaskWorkPtr work) = 0;
	virtual IKTaskThreadPool* GetThreadPool() = 0;
	virtual void Init() = 0;
	virtual void UnInit() = 0;
};

extern IKAsyncTaskManager* GetAsyncTaskManager();