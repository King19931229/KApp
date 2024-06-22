#include "KTaskGraph.h"
#include <assert.h>

KGraphTask::KGraphTask(IKTaskWorkPtr work, NamedThread::Type thread)
	: m_TaskWork(work)
	, m_TaskQueue(nullptr)
	, m_ThreadToExecuteOn(thread)
	, m_NumPrerequisite(0)
{
	m_DoneEvent = decltype(m_DoneEvent)(new KEvent());
	m_Done = false;
}

KGraphTask::KGraphTask(KGraphTask& stealFrom)
{
	assert(!stealFrom.m_TaskQueue);
	assert(stealFrom.m_NumPrerequisite == 0);

	m_TaskWork = stealFrom.m_TaskWork;
	stealFrom.m_TaskWork = nullptr;

	m_TaskQueue = stealFrom.m_TaskQueue;
	stealFrom.m_TaskQueue = nullptr;
	
	m_DoneEvent = stealFrom.m_DoneEvent;
	stealFrom.m_DoneEvent = nullptr;

	m_Done = stealFrom.m_Done;

	m_ThreadToExecuteOn = stealFrom.m_ThreadToExecuteOn;

	m_Subsequents = std::move(stealFrom.m_Subsequents);
	m_EventToWaitFor = std::move(stealFrom.m_EventToWaitFor);

	m_NumPrerequisite.store(stealFrom.m_NumPrerequisite.load());
}

KGraphTask::~KGraphTask()
{
	Abandon();
}

void KGraphTask::Setup(IKGraphTaskRef task, const std::vector<IKGraphTaskRef>& prerequisites, bool hold)
{
	((KGraphTask*)task.Get())->m_NumPrerequisite.fetch_add((uint32_t)hold + (uint32_t)prerequisites.size());
	((KGraphTask*)task.Get())->SetupPrerequisite(prerequisites);
}

void KGraphTask::SetupPrerequisite(const std::vector<IKGraphTaskRef>& prerequisites)
{
#if KTASK_GRAPH_DEBUG_PRINT_LEVEL
	printf("[GraphTask] SetupPrerequisite %s\n", GetDebugInfo());
#endif
	for (IKGraphTaskRef prerequisite : prerequisites)
	{
		if (!prerequisite || !prerequisite->AddSubsequent(this))
		{
			m_NumPrerequisite.fetch_sub(1);
		}
		else
		{
#if KTASK_GRAPH_DEBUG_PRINT_LEVEL
			printf("\t[GraphTask] AddSubsequent %s->%s\n", prerequisite->GetDebugInfo(), GetDebugInfo());
#endif
		}
	}

	if (m_NumPrerequisite.load() == 0)
	{
		QueueTask();
	}
}

bool KGraphTask::AddEventToWaitFor(IKGraphTaskRef eventToWait)
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);

	if (!m_Done)
	{
		if (std::find(m_EventToWaitFor.begin(), m_EventToWaitFor.end(), eventToWait) == m_EventToWaitFor.end())
		{
			m_EventToWaitFor.push_back(eventToWait);
			return true;
		}
	}
	return false;
}

bool KGraphTask::AddSubsequent(IKGraphTask* subsequent)
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);

	if (!m_Done)
	{
		if (std::find(m_Subsequents.begin(), m_Subsequents.end(), subsequent) == m_Subsequents.end())
		{
			m_Subsequents.push_back(subsequent);
			return true;
		}
	}
	return false;
}

bool KGraphTask::SetThreadToExetuceOn(NamedThread::Type thread)
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
	return m_Done;
}

void KGraphTask::WaitForCompletion()
{
	if (!m_Done)
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
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
	for (IKGraphTaskRef eventToWaitFor : m_EventToWaitFor)
	{
		if (!eventToWaitFor->IsCompleted())
		{
			std::vector<IKGraphTaskRef> prerequisites;
			IKGraphTaskRef noneTask = IKGraphTaskRef(new KGraphTask(IKTaskWorkPtr(new KEmptyTaskWork()), NamedThread::ANY_THREAD));
			IKGraphTaskRef newTask = IKGraphTaskRef(new KGraphTask(*this));
			KGraphTask::Setup(newTask, { noneTask }, true);
			((KGraphTask*)newTask.Get())->Dispatch();
			return;
		}
	}
	m_EventToWaitFor.clear();
	GetTaskGraphManager()->AddTask(this, m_ThreadToExecuteOn);
}

void KGraphTask::DoWork()
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
	m_TaskQueue = nullptr;
	if (m_TaskWork)
	{
		m_TaskWork->DoWork();
		m_Done = true;
		m_DoneEvent->Notify();
		DispatchSubsequents(false);
	}
}

void KGraphTask::Abandon()
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
	if (m_TaskQueue)
	{
		m_TaskQueue->RemoveTask(this);
		m_TaskQueue = nullptr;
	}
	if (m_TaskWork)
	{
		m_TaskWork->Abandon();
		m_TaskWork = nullptr;
	}
	m_Done = true;
	m_DoneEvent->Notify();
	DispatchSubsequents(true);
}

const char* KGraphTask::GetDebugInfo()
{
	if (m_TaskWork)
	{
		return m_TaskWork->GetDebugInfo();
	}
	return "NoTaskWork";
}

void KGraphTask::DispatchSubsequents(bool prerequisiteAbandon)
{
	for (IKGraphTaskRef subsequent : m_Subsequents)
	{
		((KGraphTask*)subsequent.Get())->ConditionalQueueTask(prerequisiteAbandon);
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