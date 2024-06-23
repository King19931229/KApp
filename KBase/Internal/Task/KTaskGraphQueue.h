#pragma once
#include "Interface/Task/IKTaskGraph.h"
#include "Publish/KSystem.h"
#include <list>
#include <vector>

class KGraphTaskQueue : public IKGraphTaskQueue
{
protected:
	std::vector<std::list<IKGraphTaskRef>> m_PriorityTasks;
	std::mutex m_QueueMutex;
	bool m_Done;
public:
	KGraphTaskQueue()
		: m_Done(false)
	{
	}

	~KGraphTaskQueue()
	{
	}

	void Init() override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		m_PriorityTasks.resize(TASK_PRIORITY_NUM);
		m_Done = false;
	}

	void UnInit() override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		m_PriorityTasks.clear();
		m_Done = true;
	}

	bool IsDone() const override
	{
		return m_Done;
	}

	void AddTask(IKGraphTaskRef task, TaskPriority priority) override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		if (priority >= m_PriorityTasks.size())
			return;
		m_PriorityTasks[priority].push_back(task);
#if KTASK_GRAPH_DEBUG_PRINT_LEVEL > 1
		printf("[GraphTaskQueue] AddTask %s (priority:%d)\n", task->GetDebugInfo(), priority);
#endif
	}

	void RemoveTask(IKGraphTaskRef task) override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		for (auto& taskPriorityQueue : m_PriorityTasks)
		{
			taskPriorityQueue.remove(task);
		}
#if KTASK_GRAPH_DEBUG_PRINT_LEVEL > 1
		printf("[GraphTaskQueue] RemoveTask %s\n", task->GetDebugInfo());
#endif
	}

	bool HasTaskToProcess()
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		for (auto& taskPriorityQueue : m_PriorityTasks)
		{
			if (!taskPriorityQueue.empty())
			{
				return true;
			}
		}
		return false;
	}

	IKGraphTaskRef PopTaskByPriorityOrder()
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		IKGraphTaskRef task;
		for (auto& taskPriorityQueue : m_PriorityTasks)
		{
			if (!taskPriorityQueue.empty())
			{
				task = taskPriorityQueue.front();
				taskPriorityQueue.pop_front();
				break;
			}
		}
		return task;
	}
};