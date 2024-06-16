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

	m_TaskThreadNumPerPriority = std::max(1U, ((std::thread::hardware_concurrency() - NamedThread::NUM_INTERNAL_THREAD)/ TASK_THREAD_PRIORITY_NUM));
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
}

IKGraphTaskPtr KTaskGraphManager::CreateAndDispatch(IKTaskWorkPtr task, NamedThread::Type thread, const std::vector<IKGraphTaskPtr>& prerequisites)
{
	return IKGraphTaskPtr(new KGraphTask(task, thread, prerequisites, false));
}

IKGraphTaskPtr KTaskGraphManager::CreateAndHold(IKTaskWorkPtr task, NamedThread::Type thread, const std::vector<IKGraphTaskPtr>& prerequisites)
{
	return IKGraphTaskPtr(new KGraphTask(task, thread, prerequisites, true));
}

void KTaskGraphManager::AddTask(IKGraphTask* task, NamedThread::Type thread)
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
		uint32_t threadBegin = (threadPriority == 0) ? 0 : ((threadPriority - 1) * m_TaskThreadNumPerPriority);
		uint32_t threadEnd = (threadPriority == 0) ? (m_TaskThreadNumPerPriority * TASK_THREAD_PRIORITY_NUM) : (threadPriority * m_TaskThreadNumPerPriority);

		for (uint32_t i = threadBegin; i < threadEnd; ++i)
		{
			uint32_t index = NamedThread::NUM_INTERNAL_THREAD + i;
			if (m_ThreadGraphExecute[index]->IsHangUp())
			{
				threadIndex = index;
				break;
			}
		}
		if (threadIndex == NamedThread::ANY_THREAD)
		{
			threadIndex = threadBegin;
		}
	}

	if (threadIndex < m_ThreadGraphExecute.size())
	{
		m_ThreadGraphExecute[threadIndex]->AddTask(task, (TaskPriority)taskPriority);
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