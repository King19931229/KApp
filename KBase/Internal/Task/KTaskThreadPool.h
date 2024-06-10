#pragma once
#include "Interface/Task/IKAsyncTask.h"
#include "Publish/KSystem.h"
#include <list>
#include <vector>
#include <mutex>
#include <condition_variable>

class KTaskThreadPool : public IKTaskThreadPool
{
protected:
	std::vector<std::thread> m_Workers;
	std::list<IKTaskThreadPoolWork*> m_Tasks;

	std::mutex m_QueueMutex;
	std::condition_variable m_Condition;
	bool m_Stop;

	void ThreadLoopFunction()
	{
		while (true)
		{
			IKTaskThreadPoolWork* task = nullptr;
			{
				std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
				m_Condition.wait(lock, [this] { return m_Stop || !m_Tasks.empty(); });
				if (m_Stop)
				{
					return;
				}
				task = std::move(m_Tasks.front());
				m_Tasks.pop_front();
			}
			task->DoWork();
		}
	}
public:
	KTaskThreadPool()
		: m_Stop(false)
	{
	}

	~KTaskThreadPool()
	{
		ShutDown();
	}

	void StartUp(uint32_t threadNum) override
	{
		ShutDown();
		for (uint32_t i = 0; i < threadNum; ++i)
		{
			m_Workers.emplace_back([this]()
			{
				ThreadLoopFunction();
			});
			KSystem::SetThreadName(m_Workers[i], "TaskThreadPool_Worker_" + std::to_string(i));
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
		m_Stop = false;
	}

	void PushTask(IKTaskThreadPoolWork* task) override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		m_Tasks.push_back(task);
		m_Condition.notify_one();
	}

	void PopTask(IKTaskThreadPoolWork* task) override
	{
		std::unique_lock<decltype(m_QueueMutex)> lock(m_QueueMutex);
		m_Tasks.remove(task);
		m_Condition.notify_one();
	}
};