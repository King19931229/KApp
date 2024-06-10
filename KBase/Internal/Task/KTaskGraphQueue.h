#pragma once
#include "Interface/Task/IKTaskGraph.h"
#include "Publish/KSystem.h"
#include <list>
#include <vector>
#include <mutex>
#include <condition_variable>

class KGraphTaskQueue : public IKGraphTaskQueue
{
protected:
	std::vector<std::thread> m_Workers;
	std::vector<std::list<IKGraphTask*>> m_PriorityTasks;

	std::mutex m_QueueMutex;
	std::condition_variable m_Condition;
	bool m_Stop;

	bool HasTaskToProcess()
	{
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

	void ThreadLoopFunction()
	{
		while (true)
		{
			IKGraphTask* task = nullptr;
			{
				std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
				m_Condition.wait(lock, [this] { return m_Stop || HasTaskToProcess(); });
				if (m_Stop)
				{
					return;
				}
				task = PopTaskByPriorityOrder();
			}
			task->DoWork();
		}
	}

	const char* TaskThreadPriorityName(TaskThreadPriority threadPriority)
	{
		switch (threadPriority)
		{
			case TASK_THREAD_PRIORITY_LOW:
				return "Low";
			case TASK_THREAD_PRIORITY_MEDIUM:
				return "Medium";
			case TASK_THREAD_PRIORITY_HIGH:
				return "High";
			default:
				return "";
		}
	}
public:
	KGraphTaskQueue()
		: m_Stop(false)
	{
	}

	~KGraphTaskQueue()
	{
		ShutDown();
	}

	void StartUp(TaskThreadPriority threadPriority, uint32_t numTaskPriority, uint32_t threadNum) override
	{
		ShutDown();
		m_PriorityTasks.resize(numTaskPriority);
		for (uint32_t i = 0; i < threadNum; ++i)
		{
			m_Workers.emplace_back([this]()
			{
				ThreadLoopFunction();
			});
			KSystem::SetThreadName(m_Workers[i], "TaskGraphThread_" + std::string(TaskThreadPriorityName(threadPriority)) + "_" + std::to_string(i));
		}
	}

	void ShutDown() override
	{
		{
			std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
			m_Stop = true;
		}
		m_Condition.notify_all();
		for (std::thread& worker : m_Workers)
		{
			worker.join();
		}
		m_Workers.clear();
		m_PriorityTasks.clear();
		m_Stop = false;
	}

	void PushTask(IKGraphTask* task) override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		m_PriorityTasks[0].push_back(task);
		m_Condition.notify_one();
	}

	void PopTask(IKGraphTask* task) override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		for (auto& taskPriorityQueue : m_PriorityTasks)
		{
			taskPriorityQueue.remove(task);
		}
		m_Condition.notify_one();
	}
};