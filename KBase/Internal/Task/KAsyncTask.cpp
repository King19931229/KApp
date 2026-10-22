#include "KAsyncTask.h"
#include "KAsyncTaskManager.h"
#include <assert.h>

KAsyncTask::KAsyncTask(IKTaskWorkPtr task)
	: m_TaskWork(task)
	, m_ThreadPool(nullptr)
	, m_WorkCounter(0)
	, m_Completed(task == nullptr)
{}

KAsyncTask::~KAsyncTask()
{
}

bool KAsyncTask::IsCompleted() const
{
	return m_Completed.load(std::memory_order_acquire);
}

void KAsyncTask::WaitForCompletion()
{
	if (!IsCompleted())
	{
		if (m_ThreadPool)
		{
			m_WorkFinishEvent.Wait();
			while (!IsCompleted())
			{
				std::this_thread::yield();
			}
		}
		else
		{
			DoWork();
		}
	}
}

void KAsyncTask::Abandon()
{
	if (m_ThreadPool)
	{
		m_ThreadPool->PopTask(this);
	}
	if (m_TaskWork)
	{
		m_TaskWork->Abandon();
		m_TaskWork = nullptr;
	}
	if (m_ThreadPool)
	{
		m_WorkFinishEvent.Notify();
	}
	m_Completed.store(true, std::memory_order_release);
}

void KAsyncTask::StartAsync()
{
	if (!IsCompleted() && m_WorkCounter.load() == 0)
	{
		m_WorkCounter.fetch_add(1);
		m_ThreadPool = GetAsyncTaskManager()->GetThreadPool();
		m_ThreadPool->PushTask(this);
	}
}

void KAsyncTask::StartSync()
{
	WaitForCompletion();
}

void KAsyncTask::DoWork()
{
	assert(m_TaskWork);
	m_TaskWork->DoWork();
	m_TaskWork = nullptr;
	if (m_ThreadPool)
	{
		m_WorkCounter.fetch_add(-1);
		m_WorkFinishEvent.Notify();
	}
	m_Completed.store(true, std::memory_order_release);
}