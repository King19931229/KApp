#include "KTaskGraph.h"
#include <assert.h>

KReferenceUniqueCheckData<IKGraphTask*> KReferenceUniqueCheck<IKGraphTask*, true>::ms_CheckData;
KReferenceUniqueCheckData<IKGraphTaskEvent*> KReferenceUniqueCheck<IKGraphTaskEvent*, true>::ms_CheckData;

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
	KGraphTaskEvent* taskEvent = (KGraphTaskEvent*)m_TaskEvent.Get();
	int32_t expected = KGraphTaskEvent::GTS_IDLE;
	if (!taskEvent->m_State->compare_exchange_strong(expected, KGraphTaskEvent::GTS_RUNNING, std::memory_order_acq_rel, std::memory_order_acquire))
	{
		return;
	}
	if (m_TaskWork)
	{
		m_TaskWork->DoWork();
#if KTASK_GRAPH_DEBUG_PRINT_LEVEL
		printf("[GraphTask] Done %s\n", GetDebugInfo());
#endif
		m_TaskWork = nullptr;
	}
	m_TaskQueue = nullptr;
	taskEvent->OnTaskDone();
}

void KGraphTask::DetachFromEvent()
{
	KGraphTaskEvent* taskEvent = (KGraphTaskEvent*)m_TaskEvent.Get();
	int32_t expected = KGraphTaskEvent::GTS_IDLE;
	if (!taskEvent->m_State->compare_exchange_strong(expected, KGraphTaskEvent::GTS_RUNNING, std::memory_order_acq_rel, std::memory_order_acquire))
	{
		taskEvent->WaitForCompletion();
		return;
	}
	Abandon(taskEvent->m_Task);
}

void KGraphTask::Abandon(IKGraphTaskRef thisTask)
{
	assert(thisTask.Get() == this);
	if (thisTask.Get() != this)
	{
		return;
	}
	IKGraphTaskQueue* taskQueue = m_TaskQueue.exchange(nullptr);
	if (taskQueue)
	{
		taskQueue->RemoveTask(thisTask);
	}
	if (m_TaskWork)
	{
		m_TaskWork->Abandon();
		m_TaskWork = nullptr;
	}
	((KGraphTaskEvent*)m_TaskEvent.Get())->OnTaskDone();
}

const char* KGraphTask::GetDebugInfo()
{
	return m_TaskWork ? m_TaskWork->GetDebugInfo() : "TaskWorkIsNULL";
}

KGraphTaskEvent::KGraphTaskEvent(NamedThread::Type thread)
	: m_ThreadToExecuteOn(thread)
	, m_DoneEvent(std::make_shared<KEvent>())
	, m_State(std::make_shared<std::atomic<int32_t>>(GTS_IDLE))
	, m_Queued(false)
	, m_NumPrerequisite(0)
{
}

KGraphTaskEvent::KGraphTaskEvent(KGraphTaskEvent& stealFrom)
	: m_ThreadToExecuteOn(stealFrom.m_ThreadToExecuteOn)
	, m_DoneEvent(stealFrom.m_DoneEvent)
	, m_State(stealFrom.m_State)
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
	printf("[GraphTask] SetupPrerequisite %s\n", taskEvent->GetDebugInfo());
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
			printf("\t[GraphTask] AddSubsequent %s->%s\n", prerequisite->GetDebugInfo(), taskEvent->GetDebugInfo());
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

	if (!IsCompleted() && !m_Queued)
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

	if (!IsCompleted())
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
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
	if (m_Task && !IsCompleted())
	{
		m_ThreadToExecuteOn = thread;
		return true;
	}
	return false;
}

bool KGraphTaskEvent::IsCompleted() const
{
	return m_State->load(std::memory_order_acquire) == GTS_DONE;
}

void KGraphTaskEvent::WaitForCompletion()
{
	m_DoneEvent->Wait();
}

void KGraphTaskEvent::Dispatch()
{
	ConditionalQueueTask(false);
}

void KGraphTaskEvent::OnTaskDone()
{
	int32_t expected = GTS_RUNNING;
	if (m_State->compare_exchange_strong(expected, GTS_DONE, std::memory_order_acq_rel, std::memory_order_acquire))
	{
		DispatchSubsequents(false);
		m_DoneEvent->Notify();
	}
}

void KGraphTaskEvent::QueueTask()
{
	IKGraphTaskRef task;
	IKGraphTaskEventRef newTask;
	NamedThread::Type thread = NamedThread::ANY_THREAD;
	{
		std::lock_guard<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
		if (m_Queued || IsCompleted() || !m_Task)
		{
			return;
		}
		for (IKGraphTaskEventRef eventToWaitFor : m_EventToWaitFor)
		{
			if (!eventToWaitFor->IsCompleted())
			{
				newTask = IKGraphTaskEventRef(new KGraphTaskEvent(*this));
				break;
			}
		}
		if (!newTask)
		{
			if (m_EventToWaitFor.size())
			{
				m_EventToWaitFor.clear();
			}
			task = m_Task;
			thread = m_ThreadToExecuteOn;
			m_Queued = true;
		}
	}
	if (newTask)
	{
		IKGraphTaskEventRef noneTask = IKGraphTaskEventRef(new KGraphTaskEvent(NamedThread::ANY_THREAD));
		KGraphTaskEvent::Setup(noneTask, IKTaskWorkPtr(new KEmptyTaskWork()), {}, true);
		KGraphTaskEvent::Setup(newTask, { noneTask }, false);
		((KGraphTaskEvent*)noneTask.Get())->Dispatch();
		return;
	}
	GetTaskGraphManager()->AddTask(task, thread);
#if KTASK_GRAPH_DEBUG_PRINT_LEVEL
	printf("[GraphTask] Queue %s\n", GetDebugInfo());
#endif
}

void KGraphTaskEvent::Abandon()
{
	int32_t expected = GTS_IDLE;
	if (m_State->compare_exchange_strong(expected, GTS_RUNNING, std::memory_order_acq_rel, std::memory_order_acquire))
	{
		if (m_Task)
		{
			((KGraphTask*)m_Task.Get())->Abandon(m_Task);
		}
		else
		{
			OnTaskDone();
		}
	}
	else
	{
		WaitForCompletion();
	}
}

const char* KGraphTaskEvent::GetDebugInfo()
{
	std::unique_lock<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
	if (m_Task && !IsCompleted())
	{
		return m_Task->GetDebugInfo();
	}
	return "NoTaskWork";
}

void KGraphTaskEvent::DispatchSubsequents(bool prerequisiteAbandon)
{
	IKGraphTaskRef task;
	std::vector<IKGraphTaskEventRef> subsequents;
	{
		std::lock_guard<decltype(m_TaskProcessLock)> lock(m_TaskProcessLock);
		task = std::move(m_Task);
		subsequents = std::move(m_Subsequents);
	}
	for (IKGraphTaskEventRef subsequent : subsequents)
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