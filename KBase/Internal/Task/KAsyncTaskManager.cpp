#include "KAsyncTaskManager.h"
#include "KAsyncTask.h"

KAsyncTaskManager GAsyncTaskManager;

KAsyncTaskManager::KAsyncTaskManager()
{
}

KAsyncTaskManager::~KAsyncTaskManager()
{
}

IKAsyncTaskRef KAsyncTaskManager::CreateAsyncTask(IKTaskWorkPtr work)
{
	IKAsyncTaskRef ref = IKAsyncTaskRef(new KAsyncTask(work));
	return ref;
}

IKTaskThreadPool* KAsyncTaskManager::GetThreadPool()
{
	return &m_ThreadPool;
}

struct KParallelForTask
{
	std::function<void(uint32_t)> body;
	std::atomic_uint32_t& batchIndexCounter;
	std::atomic_uint32_t& remainTaskNumCounter;
	KEvent& finishEvent;
	uint32_t batchSize;
	uint32_t taskNum;

	KParallelForTask(std::function<void(uint32_t)>& inBody,
		std::atomic_uint32_t& inBatchIndexCounter,
		std::atomic_uint32_t& inRemainTaskNumCounter,
		KEvent& inFinishEvent,
		uint32_t inBatchSize,
		uint32_t inTaskNum)
		: body(inBody)
		, batchIndexCounter(inBatchIndexCounter)
		, remainTaskNumCounter(inRemainTaskNumCounter)
		, finishEvent(inFinishEvent)
		, batchSize(inBatchSize)
		, taskNum(inTaskNum)
	{
	}

	void DoWork()
	{
		while (true)
		{
			uint32_t batchIndex = batchIndexCounter.fetch_add(1);
			uint32_t startIndex = batchIndex * batchSize;			

			if (startIndex >= taskNum)
			{
				return;
			}

			uint32_t endIndex = std::min(taskNum, (batchIndex + 1) * batchSize);

			for (uint32_t index = startIndex; index < endIndex; ++index)
			{
				body(index);
				if (remainTaskNumCounter.fetch_sub(1) == 1)
				{
					finishEvent.Notify();
				}
			}
		}
	}
	void Abandon() {}

	const char* GetDebugInfo()
	{
		return "ParallelForTask";
	}
};

void KAsyncTaskManager::ParallelFor(std::function<void(uint32_t)> body, uint32_t taskNum)
{
	if (taskNum == 0)
	{
		return;
	}

	uint32_t numWorker = m_ThreadPool.GetNumWorkers();
	// +1 For this thread local execution
	++numWorker;

	uint32_t batchSize = 1;
	uint32_t numBatch = numWorker;

	if (true)
	{
		batchSize = std::max(1U, (taskNum + (numWorker / 2)) / numWorker);
		numBatch = (taskNum + batchSize - 1) / batchSize;
	}
	else
	{
		for (uint32_t i = 6; i >= 1; --i)
		{
			uint32_t div = i * numWorker;
			batchSize = std::max(1U, (taskNum + (div / 2)) / div);
			numBatch = (taskNum + batchSize - 1) / batchSize;
			if (numBatch >= numWorker)
			{
				break;
			}
		}
	}

	std::atomic_uint32_t batchIndexCounter = 0;
	std::atomic_uint32_t remainTaskNumCounter = taskNum;
	KEvent finishEvent;

	std::vector<IKAsyncTaskRef> workerTasks;
	workerTasks.resize(numWorker);

	for (uint32_t workerIndex = 0; workerIndex < numWorker; ++workerIndex)
	{
		IKAsyncTaskRef workerTask = CreateAsyncTask(IKTaskWorkPtr(new KTaskWork<KParallelForTask>(body, batchIndexCounter, remainTaskNumCounter, finishEvent, batchSize, taskNum)));
		workerTasks[workerIndex] = workerTask;

		if (workerIndex != numWorker - 1)
		{
			workerTask->StartAsync();
		}
		else
		{
			workerTask->StartSync();
		}
	}

	finishEvent.Wait();
}

void KAsyncTaskManager::Init()
{
	m_ThreadPool.StartUp(std::max(2U, std::thread::hardware_concurrency() - 1U));
}

void KAsyncTaskManager::UnInit()
{
	m_ThreadPool.ShutDown();
}

IKAsyncTaskManager* GetAsyncTaskManager()
{
	return &GAsyncTaskManager;
}