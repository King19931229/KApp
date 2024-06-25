#include "KTaskGraph.h"
#include <assert.h>

KGraphTask::KGraphTask(IKGraphTaskEventRef taskEvent, IKTaskWorkPtr taskWork, IKGraphTaskQueue* taskQueue)
	: m_TaskEvent(taskEvent)
	, m_TaskWork(taskWork)
	, m_TaskQueue(taskQueue)
{
}

KGraphTask::~KGraphTask()
{
}

void KGraphTask::SetTaskQueue(IKGraphTaskQueue* queue)
{
	m_TaskQueue = queue;
}

void KGraphTask::DoWork()
{
	std::unique_lock<decltype(m_WorkProcessLock)> lock(m_WorkProcessLock);
	if (m_TaskWork)
	{
		m_TaskWork->DoWork();
		if (m_TaskEvent)
		{
			((KGraphTaskEvent*)m_TaskEvent.Get())->OnTaskDone();
		}
	}
	m_TaskQueue = nullptr;
	m_TaskEvent.Release();
}

void KGraphTask::DetachFromEvent()
{
	std::unique_lock<decltype(m_WorkProcessLock)> lock(m_WorkProcessLock);
	m_TaskWork = nullptr;
	m_TaskQueue = nullptr;
	m_TaskEvent.Release();
}

void KGraphTask::Abandon(IKGraphTaskRef thisTask)
{
	std::unique_lock<decltype(m_WorkProcessLock)> lock(m_WorkProcessLock);
	assert(thisTask.Get() == this);
	if (thisTask.Get() != this)
	{
		return;
	}
	if (m_TaskQueue)
	{
		m_TaskQueue->RemoveTask(thisTask);
		m_TaskQueue = nullptr;
	}
	if (m_TaskWork)
	{
		m_TaskWork->Abandon();
		m_TaskWork = nullptr;
		if (m_TaskEvent)
		{
			((KGraphTaskEvent*)m_TaskEvent.Get())->OnTaskDone();
		}
	}
	m_TaskEvent.Release();
}

const char* KGraphTask::GetDebugInfo()
{
	return m_TaskWork ? m_TaskWork->GetDebugInfo() : "TaskWorkIsNULL";
}

KGraphTaskEvent::KGraphTaskEvent(NamedThread::Type thread)
	: m_ThreadToExecuteOn(thread)
	, m_DoneEvent(std::make_shared<KEvent>())
	, m_Done(std::make_shared<bool>(false))
	, m_Queued(false)
{
}

KGraphTaskEvent::KGraphTaskEvent(KGraphTaskEvent& stealFrom)
	: m_ThreadToExecuteOn(stealFrom.m_ThreadToExecuteOn)
	, m_DoneEvent(stealFrom.m_DoneEvent)
	, m_Done(stealFrom.m_Done)
	, m_Queued(false)
	, m_Task(stealFrom.m_Task)
	, m_Subsequents(std::move(stealFrom.m_Subsequents))
	, m_EventToWaitFor(std::move(stealFrom.m_EventToWaitFor))
{
	m_NumPrerequisite.store(stealFrom.m_NumPrerequisite.load());
}

KGraphTaskEvent::~KGraphTaskEvent()
{
}

void KGraphTaskEvent::Setup(IKGraphTaskEventRef taskEvent, IKTaskWorkPtr work, const std::vector<IKGraphTaskEventRef>& prerequisites, bool hold)
{
	IKGraphTaskRef task = IKGraphTaskRef(new KGraphTask(taskEvent, work, nullptr));
	KGraphTaskEvent* graphTaskEvent = (KGraphTaskEvent*)taskEvent.Get();
	graphTaskEvent->m_NumPrerequisite.fetch_add((uint32_t)hold + (uint32_t)prerequisites.size());
	graphTaskEvent->m_Task = task;
	KGraphTaskEvent::SetupPrerequisite(taskEvent, prerequisites);
}

void KGraphTaskEvent::Setup(IKGraphTaskEventRef taskEvent, const std::vector<IKGraphTaskEventRef>& prerequisites, bool hold)
{
	KGraphTaskEvent* graphTaskEvent = (KGraphTaskEvent*)taskEvent.Get();
	assert(graphTaskEvent->m_Task);
	graphTaskEvent->m_NumPrerequisite.fetch_add((uint32_t)hold + (uint32_t)prerequisites.size());
	KGraphTaskEvent::SetupPrerequisite(taskEvent, prerequisites);
}

void KGraphTaskEvent::SetupPrerequisite(IKGraphTaskEventRef taskEvent, const std::vector<IKGraphTaskEventRef>& prerequisites)
{
	KGraphTaskEvent* graphTaskEvent = (KGraphTaskEvent*)taskEvent.Get();
#if KTASK_GRAPH_DEBUG_PRINT_LEVEL
	printf("[GraphTask] SetupPrerequisite %s\n", GetDebugInfo());
#endif
	for (IKGraphTaskEventRef prerequisite : prerequisites)
	{
		if (!prerequisite || !((KGraphTaskEvent*)prerequisite.Get())->AddSubsequent(taskEvent))
		{
			graphTaskEvent->m_NumPrerequisite.fetch_sub(1);
		}
		else
		{
#if KTASK_GRAPH_DEBUG_PRINT_LEVEL
			printf("\t[GraphTask] AddSubsequent %s->%s\n", prerequisite->GetDebugInfo(), GetDebugInfo());
#endif
		}
	}

	if (graphTaskEvent->m_NumPrerequisite.load() == 0)
	{
		graphTaskEvent->QueueTask();
	}
}

bool KGraphTaskEvent::AddEventToWaitFor(IKGraphTaskEventRef eventToWait)
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);

	if (!*m_Done && !m_Queued)
	{
		if (std::find(m_EventToWaitFor.begin(), m_EventToWaitFor.end(), eventToWait) == m_EventToWaitFor.end())
		{
			m_EventToWaitFor.push_back(eventToWait);
			return true;
		}
	}
	return false;
}

bool KGraphTaskEvent::AddSubsequent(IKGraphTaskEventRef subsequent)
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);

	if (!*m_Done)
	{
		if (std::find(m_Subsequents.begin(), m_Subsequents.end(), subsequent) == m_Subsequents.end())
		{
			m_Subsequents.push_back(subsequent);
			return true;
		}
	}
	return false;
}

bool KGraphTaskEvent::SetThreadToExetuceOn(NamedThread::Type thread)
{
	if (m_Task)
	{
		m_ThreadToExecuteOn = thread;
		return true;
	}
	return false;
}

bool KGraphTaskEvent::IsCompleted() const
{
	return *m_Done;
}

void KGraphTaskEvent::WaitForCompletion()
{
	if (!*m_Done)
	{
		m_DoneEvent->Wait();
	}
}

void KGraphTaskEvent::Dispatch()
{
	ConditionalQueueTask(false);
}

void KGraphTaskEvent::QueueTask()
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
	assert(m_Task);
	for (IKGraphTaskEventRef eventToWaitFor : m_EventToWaitFor)
	{
		if (!eventToWaitFor->IsCompleted())
		{
			std::vector<IKGraphTaskRef> prerequisites;
			IKGraphTaskEventRef noneTask = IKGraphTaskEventRef(new KGraphTaskEvent(NamedThread::ANY_THREAD));
			IKGraphTaskEventRef newTask = IKGraphTaskEventRef(new KGraphTaskEvent(*this));
			KGraphTaskEvent::Setup(noneTask, IKTaskWorkPtr(new KEmptyTaskWork()), {}, true);
			KGraphTaskEvent::Setup(newTask, { noneTask }, false);
			((KGraphTaskEvent*)noneTask.Get())->Dispatch();
			return;
		}
	}
	if (m_EventToWaitFor.size())
	{
		m_EventToWaitFor.clear();
	}
	GetTaskGraphManager()->AddTask(m_Task, m_ThreadToExecuteOn);
	m_Queued = true;
}

void KGraphTaskEvent::Abandon()
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
	if (m_Task)
	{
		((KGraphTask*)m_Task.Get())->Abandon(m_Task);
		m_Task.Release();
	}
}

const char* KGraphTaskEvent::GetDebugInfo()
{
	if (m_Task)
	{
		return m_Task->GetDebugInfo();
	}
	return "NoTaskWork";
}

void KGraphTaskEvent::OnTaskDone()
{
	if (m_Task)
	{
		*m_Done = true;
		m_DoneEvent->Notify();
		DispatchSubsequents(false);
	}
	m_Task.Release();
}

void KGraphTaskEvent::DispatchSubsequents(bool prerequisiteAbandon)
{
	for (IKGraphTaskEventRef subsequent : m_Subsequents)
	{
		((KGraphTaskEvent*)subsequent.Get())->ConditionalQueueTask(prerequisiteAbandon);
	}
}

void KGraphTaskEvent::ConditionalQueueTask(bool prerequisiteDone)
{
	if (m_NumPrerequisite.fetch_sub(1) == 1)
	{
		QueueTask();
	}
}

void KGraphTaskEvent::PrerequisiteComplete(int32_t num)
{
	if (m_NumPrerequisite.fetch_sub(num) == num)
	{
		QueueTask();
	}
}