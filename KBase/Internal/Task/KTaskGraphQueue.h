#pragma once
#include "Interface/Task/IKTaskGraph.h"
#include "Publish/KSystem.h"
#include <list>
#include <vector>

class KGraphTaskQueue : public IKGraphTaskQueue
{
protected:
	std::vector<std::list<IKGraphTask*>> m_PriorityTasks;
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

	void AddTask(IKGraphTask* task, TaskPriority priority) override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		m_PriorityTasks[priority].push_back(task);
	}

	void RemoveTask(IKGraphTask* task) override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		for (auto& taskPriorityQueue : m_PriorityTasks)
		{
			taskPriorityQueue.remove(task);
		}
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

	IKGraphTask* PopTaskByPriorityOrder()
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		IKGraphTask* task = nullptr;
		for (auto& taskPriorityQueue : m_PriorityTasks)
		{
			if (!taskPriorityQueue.empty())
			{
				task = std::move(taskPriorityQueue.front());
				taskPriorityQueue.pop_front();
				break;
			}
		}
		return task;
	}
};