#include "KAsyncTask.h"
#include "KAsyncTaskManager.h"

KAsyncTask::KAsyncTask(IKTaskWorkPtr task)
	: m_TaskWork(task)
	, m_ThreadPool(nullptr)
	, m_State(task == nullptr ? ATS_DONE : ATS_IDLE)
{
}

KAsyncTask::~KAsyncTask()
{
}

bool KAsyncTask::IsCompleted() const
{
	return m_State.load(std::memory_order_acquire) == ATS_DONE;
}

void KAsyncTask::WaitForCompletion()
{
	int32_t expected = ATS_IDLE;
	if (m_State.compare_exchange_strong(expected, ATS_RUNNING, std::memory_order_acq_rel, std::memory_order_acquire))
	{
		Execute();
		return;
	}

	if (!IsCompleted())
	{
		m_WorkFinishEvent.Wait();
	}
}

void KAsyncTask::StartAsync()
{
	IKTaskThreadPool* threadPool = GetAsyncTaskManager()->GetThreadPool();
	m_ThreadPool = threadPool;

	int32_t expected = ATS_IDLE;
	if (!m_State.compare_exchange_strong(expected, ATS_QUEUED, std::memory_order_acq_rel, std::memory_order_acquire))
	{
		return;
	}

	threadPool->PushTask(this);
}

void KAsyncTask::StartSync()
{
	WaitForCompletion();
}

void KAsyncTask::DoWork()
{
	int32_t expected = ATS_QUEUED;
	if (!m_State.compare_exchange_strong(expected, ATS_RUNNING, std::memory_order_acq_rel, std::memory_order_acquire))
	{
		return;
	}

	Execute();
}

void KAsyncTask::Execute()
{
	IKTaskWorkPtr taskWork = std::move(m_TaskWork);
	taskWork->DoWork();
	m_State.store(ATS_DONE, std::memory_order_release);
	m_WorkFinishEvent.Notify();
}

void KAsyncTask::Abandon()
{
	int32_t state = m_State.load(std::memory_order_acquire);
	while (state == ATS_IDLE || state == ATS_QUEUED)
	{
		if (m_State.compare_exchange_weak(state, ATS_DONE, std::memory_order_acq_rel, std::memory_order_acquire))
		{
			if (state == ATS_QUEUED && m_ThreadPool)
			{
				m_ThreadPool->PopTask(this);
			}

			IKTaskWorkPtr taskWork = std::move(m_TaskWork);
			taskWork->Abandon();
			m_WorkFinishEvent.Notify();
			return;
		}
	}
}