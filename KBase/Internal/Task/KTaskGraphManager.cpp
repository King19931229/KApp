#include "KTaskGraphManager.h"
#include "KTaskGraph.h"

KTaskGraphManager::KTaskGraphManager()
	: m_TaskThreadNum(0)
{}

KTaskGraphManager::~KTaskGraphManager()
{}

void KTaskGraphManager::Init()
{
	UnInit();

	m_TaskThreadNumPerPriority = std::max(1U, ((2 * std::thread::hardware_concurrency() - NamedThread::NUM_INTERNAL_THREAD) / TASK_THREAD_PRIORITY_NUM));
	m_TaskThreadNum = TASK_THREAD_PRIORITY_NUM * m_TaskThreadNumPerPriority;

	for (uint32_t i = 0; i < TASK_THREAD_PRIORITY_NUM; ++i)
	{
		m_ThreadPriorityTaskQueue[i].Init();
	}

	m_ThreadGraphExecute.resize(NamedThread::NUM_INTERNAL_THREAD + m_TaskThreadNum);
	for (uint32_t i = 0; i < NamedThread::NUM_INTERNAL_THREAD; ++i)
	{
		m_ThreadGraphExecute[i] = KTaskGraphExecutePtr(new KNamedThreadExecute());
		m_ThreadGraphExecute[i]->StartUp();
	}

	m_TaskThread.resize(m_TaskThreadNum);
	for (uint32_t priorityIndex = 0; priorityIndex < TASK_THREAD_PRIORITY_NUM; ++priorityIndex)
	{
		for (uint32_t localThreadIndex = 0; localThreadIndex < m_TaskThreadNumPerPriority; ++localThreadIndex)
		{
			uint32_t threadIndex = priorityIndex * m_TaskThreadNumPerPriority + localThreadIndex;
			uint32_t globalThreadIndex = NamedThread::NUM_INTERNAL_THREAD + threadIndex;

			std::string threadName = std::string("TaskThread") + TaskThreadPriorityName((TaskThreadPriority)priorityIndex) + std::to_string(localThreadIndex);

			KTaskGraphExecutePtr taskExecute = KTaskGraphExecutePtr(new KTaskThreadExecute(&m_ThreadPriorityTaskQueue[priorityIndex]));

			m_ThreadGraphExecute[globalThreadIndex] = taskExecute;
			m_TaskThread[threadIndex] = KRunableThreadPtr(new KRunableThread(taskExecute, threadName));
			m_TaskThread[threadIndex]->StartUp();
		}
	}
}

void KTaskGraphManager::UnInit()
{
	for (uint32_t i = 0; i < TASK_THREAD_PRIORITY_NUM; ++i)
	{
		m_ThreadPriorityTaskQueue[i].UnInit();
	}

	for (uint32_t i = 0; i < (uint32_t)m_ThreadGraphExecute.size(); ++i)
	{
		m_ThreadGraphExecute[i]->ShutDown();
	}
	m_ThreadGraphExecute.clear();

	for (uint32_t i = 0; i < (uint32_t)m_TaskThread.size(); ++i)
	{
		m_TaskThread[i]->ShutDown();
	}
	m_TaskThread.clear();

	m_TaskThreadNumPerPriority = 0;
	m_TaskThreadNum = 0;
}

IKGraphTaskEventRef KTaskGraphManager::CreateAndDispatch(IKTaskWorkPtr work, NamedThread::Type thread, const std::vector<IKGraphTaskEventRef>& prerequisites)
{
	IKGraphTaskEventRef task = IKGraphTaskEventRef(new KGraphTaskEvent(thread));
	KGraphTaskEvent::Setup(task, work, prerequisites, false);
	return task;
}

IKGraphTaskEventRef KTaskGraphManager::CreateAndHold(IKTaskWorkPtr work, NamedThread::Type thread, const std::vector<IKGraphTaskEventRef>& prerequisites)
{
	IKGraphTaskEventRef task = IKGraphTaskEventRef(new KGraphTaskEvent(thread));
	KGraphTaskEvent::Setup(task, work, prerequisites, true);
	return task;
}

void KTaskGraphManager::Dispatch(IKGraphTaskEventRef taskEvent)
{
	((KGraphTaskEvent*)taskEvent.Get())->Dispatch();
}

void KTaskGraphManager::AddTask(IKGraphTaskRef task, NamedThread::Type thread)
{
	uint32_t threadIndex = NamedThread::ANY_THREAD;
	uint32_t threadPriority = (thread & NamedThread::THREAD_PRIORITY_MASK) >> NamedThread::THREAD_PRIORITY_SHIFT;
	uint32_t taskPriority = (thread & NamedThread::TASK_PRIORITY_MASK) >> NamedThread::TASK_PRIORITY_SHIFT;

	if ((thread & NamedThread::ANY_THREAD) != 0XFF)
	{
		threadIndex = thread & NamedThread::ANY_THREAD;
	}
	else
	{
		uint32_t threadBegin = NamedThread::NUM_INTERNAL_THREAD + ((threadPriority == 0) ? 0 : ((threadPriority - 1) * m_TaskThreadNumPerPriority));
		uint32_t threadEnd = NamedThread::NUM_INTERNAL_THREAD + ((threadPriority == 0) ? (m_TaskThreadNumPerPriority * TASK_THREAD_PRIORITY_NUM) : (threadPriority * m_TaskThreadNumPerPriority));

		for (uint32_t i = threadBegin; i < threadEnd; ++i)
		{
			if (i < m_ThreadGraphExecute.size() && m_ThreadGraphExecute[i]->IsHangUp())
			{
				threadIndex = i;
				break;
			}
		}
		if (threadIndex == NamedThread::ANY_THREAD)
		{
			threadIndex = threadBegin + rand() % m_TaskThreadNum;
		}
	}

	if (threadIndex < m_ThreadGraphExecute.size())
	{
		m_ThreadGraphExecute[threadIndex]->AddTask(task, (TaskPriority)taskPriority);
	}
	else if (m_ThreadGraphExecute.size())
	{
		assert(false);
	}
}

void KTaskGraphManager::AttachToThread(NamedThread::Type thread)
{
}

void KTaskGraphManager::ProcessTaskUntilIdle(NamedThread::Type thread)
{
	if (thread < m_ThreadGraphExecute.size())
	{
		return m_ThreadGraphExecute[thread]->ProcessTaskUntilIdle();
	}
}

KTaskGraphManager GTaskGraphManager;
IKTaskGraphManager* GetTaskGraphManager()
{
	return &GTaskGraphManager;
}