#include "KTaskGraph.h"
#include <assert.h>

KGraphTask::KGraphTask(IKTaskWorkPtr work, NamedThread thread, const std::vector<IKGraphTaskPtr>& prerequisites, bool hold)
	: m_TaskWork(work)
	, m_TaskQueue(nullptr)
	, m_ThreadToExecuteOn(thread)
	, m_NumPrerequisite((uint32_t)hold + (uint32_t)prerequisites.size())
{
	m_DoneEvent = decltype(m_DoneEvent)(new KEvent());
	m_Done = decltype(m_Done)(new std::atomic_bool());
	SetupPrerequisite(prerequisites);
}

KGraphTask::KGraphTask(KGraphTask& stealFrom, const std::vector<IKGraphTaskPtr>& prerequisites, bool hold)
{
	assert(!stealFrom.m_TaskQueue);
	assert(stealFrom.m_NumPrerequisite == 0);

	m_TaskWork = stealFrom.m_TaskWork;
	stealFrom.m_TaskWork = nullptr;

	m_TaskQueue = stealFrom.m_TaskQueue;
	stealFrom.m_TaskQueue = nullptr;
	
	m_DoneEvent = stealFrom.m_DoneEvent;
	m_Done = stealFrom.m_Done;

	m_ThreadToExecuteOn = stealFrom.m_ThreadToExecuteOn;

	m_Subsequents = std::move(stealFrom.m_Subsequents);
	m_EventToWaitFor = std::move(stealFrom.m_EventToWaitFor);

	m_NumPrerequisite.store(stealFrom.m_NumPrerequisite.load());	

	m_NumPrerequisite.fetch_add((uint32_t)hold + (uint32_t)prerequisites.size());
	SetupPrerequisite(prerequisites);
}

KGraphTask::~KGraphTask()
{
	Abandon();
}

void KGraphTask::SetupPrerequisite(const std::vector<IKGraphTaskPtr>& prerequisites)
{
	for (IKGraphTaskPtr prerequisite : prerequisites)
	{
		if (!prerequisite || !prerequisite->AddSubsequent(shared_from_this()))
		{
			m_NumPrerequisite.fetch_sub(1);
		}
	}

	if (m_NumPrerequisite.load() == 0)
	{
		QueueTask();
	}
}

bool KGraphTask::AddEventToWaitFor(IKGraphTaskPtr eventToWait)
{
	if (!m_Done->load())
	{
		if (std::find(m_EventToWaitFor.begin(), m_EventToWaitFor.end(), eventToWait) == m_EventToWaitFor.end())
		{
			m_EventToWaitFor.push_back(eventToWait);
			return true;
		}
	}
	return false;
}

bool KGraphTask::AddSubsequent(IKGraphTaskPtr subsequent)
{
	if (!m_Done->load())
	{
		if (std::find(m_Subsequents.begin(), m_Subsequents.end(), subsequent) == m_Subsequents.end())
		{
			m_Subsequents.push_back(subsequent);
			return true;
		}
	}
	return false;
}

bool KGraphTask::SetThreadToExetuceOn(NamedThread thread)
{
	if (m_TaskQueue)
	{
		return false;
	}
	m_ThreadToExecuteOn = thread;
	return true;
}

bool KGraphTask::IsCompleted() const
{
	return m_Done->load();
}

void KGraphTask::WaitForCompletion()
{
	if (!m_Done->load())
	{
		m_DoneEvent->Wait();
	}
}

void KGraphTask::Dispatch()
{
	ConditionalQueueTask(false);
}

void KGraphTask::QueueTask()
{
	for (IKGraphTaskPtr eventToWaitFor : m_EventToWaitFor)
	{
		if (!eventToWaitFor->IsCompleted())
		{
			std::vector<IKGraphTaskPtr> prerequisites;
			IKGraphTaskPtr noneTask = IKGraphTaskPtr(new KGraphTask(IKTaskWorkPtr(new KEmptyTaskWork()), ANY_THREAD, {}, false));
			IKGraphTaskPtr newTask = IKGraphTaskPtr(new KGraphTask(*this, { noneTask }, true));
			((KGraphTask*)newTask.get())->Dispatch();
			return;
		}
	}

	IKGraphTaskQueue* queue = GetTaskGraphManager()->GetQueue(m_ThreadToExecuteOn);
	queue->PushTask(this);
}

void KGraphTask::DoWork()
{
	m_TaskWork->DoWork();
	m_TaskWork = nullptr;
	m_TaskQueue = nullptr;
	m_Done->store(true);
	m_DoneEvent->Notify();
	DispatchSubsequents(false);
}

void KGraphTask::Abandon()
{
	if (m_TaskQueue)
	{
		m_TaskQueue->PopTask(this);
		m_TaskQueue = nullptr;
	}
	if (m_TaskWork)
	{
		m_TaskWork->Abandon();
		m_TaskWork = nullptr;
		m_Done->store(true);
		m_DoneEvent->Notify();
		DispatchSubsequents(true);
	}
}

void KGraphTask::DispatchSubsequents(bool prerequisiteAbandon)
{
	for (IKGraphTaskPtr subsequent : m_Subsequents)
	{
		((KGraphTask*)subsequent.get())->ConditionalQueueTask(prerequisiteAbandon);
	}
}

void KGraphTask::ConditionalQueueTask(bool prerequisiteDone)
{
	if (m_NumPrerequisite.fetch_sub(1) == 1)
	{
		QueueTask();
	}
}

void KGraphTask::PrerequisiteComplete(int32_t num)
{
	if (m_NumPrerequisite.fetch_sub(num) == num)
	{
		QueueTask();
	}
}