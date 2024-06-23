#pragma once
#include "Interface/IKRunable.h"
#include "KTaskGraphQueue.h"
#include "KTaskGraph.h"
#include <assert.h>

class KTaskGraphExecute : public IKRunable
{
protected:
	KEvent m_Trigger;
	KGraphTaskQueue* m_Queue;
	bool m_HangUp;
public:
	KTaskGraphExecute()
		: m_Queue(nullptr)
		, m_HangUp(false)
	{}

	void StartUp() override
	{
	}

	void ShutDown() override
	{
		m_Trigger.Notify();
	}

	bool IsHangUp() const { return m_HangUp; }

	void AddTask(IKGraphTaskRef task, TaskPriority taskPriority)
	{
		m_Queue->AddTask(task, taskPriority);
		((KGraphTask*)task.Get())->SetTaskQueue(m_Queue);
		m_Trigger.Notify();
		m_HangUp = false;
	}

	virtual void ProcessTaskUntilIdle() = 0;
	virtual void ProcessTaskUntilQuit() = 0;
};

typedef std::shared_ptr<KTaskGraphExecute> KTaskGraphExecutePtr;

class KNamedThreadExecute : public KTaskGraphExecute
{
protected:
public:
	void StartUp() override
	{
		if (!m_Queue)
		{
			m_Queue = new KGraphTaskQueue();
			m_Queue->Init();
		}
	}

	void Run() override
	{
	}

	void ShutDown() override
	{
		KTaskGraphExecute::ShutDown();
		if (m_Queue)
		{
			m_Queue->UnInit();
			KDELETE(m_Queue);
			m_Queue = nullptr;
		}
	}

	void ProcessTaskUntilIdle() override
	{
		while (m_Queue && !m_Queue->IsDone())
		{
			IKGraphTaskRef task = m_Queue->PopTaskByPriorityOrder();
			if (task)
			{
				task->DoWork();
				continue;
			}
			break;
		}
	}

	void ProcessTaskUntilQuit() override
	{
		while (m_Queue && !m_Queue->IsDone())
		{
			IKGraphTaskRef task = m_Queue->PopTaskByPriorityOrder();
			if (task)
			{
				task->DoWork();
				continue;
			}
			m_HangUp = true;
			std::atomic_thread_fence(std::memory_order_acq_rel);
			m_Trigger.Wait();
			std::atomic_thread_fence(std::memory_order_acq_rel);
			m_HangUp = false;
		}
	}
};

class KTaskThreadExecute : public KTaskGraphExecute
{
protected:
public:
	KTaskThreadExecute(KGraphTaskQueue* queue)
	{
		m_Queue = queue;
	}

	~KTaskThreadExecute()
	{}

	void Run() override
	{
		while (m_Queue && !m_Queue->IsDone())
		{
			IKGraphTaskRef task = m_Queue->PopTaskByPriorityOrder();
			if (task)
			{
				task->DoWork();
				continue;
			}
			m_HangUp = true;
			std::atomic_thread_fence(std::memory_order_acq_rel);
			m_Trigger.Wait();
			std::atomic_thread_fence(std::memory_order_acq_rel);
			m_HangUp = false;
		}
	}

	void ProcessTaskUntilIdle() override {}
	void ProcessTaskUntilQuit() override {}
};