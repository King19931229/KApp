#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <assert.h>

class KRenderThreadPool
{
public:
	class KRenderThread
	{
	protected:
		std::thread m_Thread;
		std::queue<std::function<void()>> m_JobQueue;
		std::mutex m_Lock;
		std::condition_variable m_Cond;
		bool m_bQuit;

		void ThreadFunc()
		{
			while(!m_bQuit)
			{
				std::function<void()> job;
				{
					std::unique_lock<std::mutex> lock(m_Lock);
					m_Cond.wait(lock, [this] { return !m_JobQueue.empty() || m_bQuit; });
					if(m_bQuit)
					{
						break;
					}
					job = m_JobQueue.front();
				}
				job();
				{
					std::unique_lock<std::mutex> lock(m_Lock);
					m_JobQueue.pop();
					m_Cond.notify_one();
				}
			}
		}
	public:
		KRenderThread()
			: m_bQuit(false)
		{
			m_Thread = std::thread(&KRenderThread::ThreadFunc, this);
		}

		~KRenderThread()
		{
			if (m_Thread.joinable())
			{
				m_bQuit = true;
				m_Cond.notify_one();
				m_Thread.join();
			}
		}

		void AddJob(std::function<void()> function)
		{
			std::lock_guard<std::mutex> lock(m_Lock);
			m_JobQueue.push(function);
			m_Cond.notify_one();
		}

		void Wait()
		{
			std::unique_lock<std::mutex> lock(m_Lock);
			m_Cond.wait(lock, [this]() { return m_JobQueue.empty() || m_bQuit; });
		}
	};
	typedef std::shared_ptr<KRenderThread> RenderThreadPtr;
protected:
	std::vector<RenderThreadPtr> m_Threads;
public:	
	KRenderThreadPool()
	{
	}

	void AddJob(size_t idx, std::function<void()> job)
	{
		assert(idx < m_Threads.size());
		if(idx < m_Threads.size())
		{
			m_Threads[idx]->AddJob(job);
		}
	}

	void SetThreadCount(size_t count)
	{
		m_Threads.clear();
		for (size_t i = 0; i < count; i++)
		{
			m_Threads.push_back(std::shared_ptr<KRenderThread>(KNEW KRenderThread()));
		}
	}

	size_t GetThreadCount()
	{
		return static_cast<uint32_t>(m_Threads.size());
	}

	void WaitAll()
	{
		for (auto &thread : m_Threads)
		{
			thread->Wait();
		}
	}

	void Wait(size_t idx)
	{
		assert(idx < m_Threads.size());
		if(idx < m_Threads.size())
		{
			m_Threads[idx]->Wait();
		}
	}
};