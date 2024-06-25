#include "KAsyncTask.h"
#include "KAsyncTaskManager.h"
#include <assert.h>

KAsyncTask::KAsyncTask(IKTaskWorkPtr task)
	: m_TaskWork(task)
	, m_ThreadPool(nullptr)
	, m_WorkCounter(0)
{}

KAsyncTask::~KAsyncTask()
{
}

bool KAsyncTask::IsCompleted() const
{
	return m_TaskWork == nullptr;
}

void KAsyncTask::WaitForCompletion()
{
	if (m_TaskWork)
	{
		if (m_ThreadPool)
		{
			m_WorkFinishEvent.Wait();
		}
		else
		{
			m_TaskWork->DoWork();
		}
		m_TaskWork = nullptr;
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
}

void KAsyncTask::StartAsync()
{
	if (m_TaskWork && m_WorkCounter.load() == 0)
	{
		m_WorkCounter.fetch_add(1);
		m_ThreadPool = GetAsyncTaskManager()->GetThreadPool();
		m_ThreadPool->PushTask(this);
	}
}

void KAsyncTask::StartSync()
{
	if (m_ThreadPool)
	{
		if (m_WorkCounter.load() == 1)
		{
			m_WorkFinishEvent.Wait();
		}
	}
	else
	{
		DoWork();
	}
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
}