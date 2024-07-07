#pragma once
#include "KBase/Interface/Task/IKTaskWork.h"
#include "KBase/Publish/KReferenceHolder.h"

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
	virtual uint32_t GetNumWorkers() const = 0;
};

struct IKAsyncTask : public IKTaskThreadPoolWork
{
	virtual ~IKAsyncTask() {}
	virtual bool IsCompleted() const = 0;
	virtual void WaitForCompletion() = 0;
	virtual void StartAsync() = 0;
	virtual void StartSync() = 0;
};

typedef KReferenceHolder<IKAsyncTask*, true> IKAsyncTaskRef;

struct IKAsyncTaskManager
{
	virtual ~IKAsyncTaskManager() {}
	virtual IKAsyncTaskRef CreateAsyncTask(IKTaskWorkPtr work) = 0;
	virtual IKTaskThreadPool* GetThreadPool() = 0;
	virtual void ParallelFor(std::function<void(uint32_t)> body, uint32_t num) = 0;
	virtual void Init() = 0;
	virtual void UnInit() = 0;
};

extern IKAsyncTaskManager* GetAsyncTaskManager();

inline void ParallelFor(std::function<void(uint32_t)> body, uint32_t num)
{
	IKAsyncTaskManager* taskManager = GetAsyncTaskManager();
	taskManager->ParallelFor(body, num);
}