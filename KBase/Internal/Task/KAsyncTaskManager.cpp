#include "KAsyncTaskManager.h"
#include "KAsyncTask.h"

KAsyncTaskManager GAsyncTaskManager;

KAsyncTaskManager::KAsyncTaskManager()
{
}

KAsyncTaskManager::~KAsyncTaskManager()
{
}

IKAsyncTaskRef KAsyncTaskManager::CreateAsyncTask(IKTaskWorkPtr work)
{
	IKAsyncTaskRef ref = IKAsyncTaskRef(new KAsyncTask(work));
	return ref;
}

IKTaskThreadPool* KAsyncTaskManager::GetThreadPool()
{
	return &m_ThreadPool;
}

void KAsyncTaskManager::Init()
{
	m_ThreadPool.StartUp(std::max(2U, std::thread::hardware_concurrency() - 1U));
}

void KAsyncTaskManager::UnInit()
{
	m_ThreadPool.ShutDown();
}

IKAsyncTaskManager* GetAsyncTaskManager()
{
	return &GAsyncTaskManager;
}