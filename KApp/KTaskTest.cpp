#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKMemory.h"
#include "KBase/Publish/KEvent.h"
#include "KBase/Interface/Task/IKAsyncTask.h"
#include "KBase/Interface/Task/IKTaskGraph.h"
#include "KBase/Internal/Task/KTaskThreadPool.h"
#include <functional>

KEvent event;

std::atomic_uint32_t counter;
uint32_t counter2;

void Task()
{
	counter.fetch_add(1);
	printf("counter:%d\n", counter.load());
}

void Task2()
{
	++counter2;
	printf("counter2:%d\n", counter2);
}


int main()
{
	DUMP_MEMORY_LEAK_BEGIN();
	/*
	std::thread t(Task);
	event.WaitFor(1000 * 2);
	printf("main Bef\n");
	event.Wait();
	printf("main Aft\n");
	t.join();
	*/

	GetAsyncTaskManager()->Init();
	GetTaskGraphManager()->Init();
	
	uint32_t taskNum = 10000;
	/*std::vector<IKAsyncTaskPtr> tasks;
	for(uint32_t i = 0; i < taskNum; ++i)
	{
		tasks.push_back(GetAsyncTaskManager()->CreateAsyncTask(IKTaskWorkPtr(new KLambdaTaskWork(Task))));
	}
	for (uint32_t i = 0; i < taskNum; ++i)
	{
		tasks[i]->StartAsync();
	}*/

	std::vector<IKGraphTaskPtr> tasks;
	for (uint32_t i = 0; i < taskNum; ++i)
	{
		tasks.push_back(GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork(Task)), NamedThread::HIGH_THREAD_LOW_TASK_ANY_THREAD, {}));
	}

	std::vector<IKGraphTaskPtr> tasks2;
	for (uint32_t i = 0; i < taskNum; ++i)
	{
		tasks2.push_back(GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork(Task2)), NamedThread::HIGH_THREAD_LOW_TASK_ANY_THREAD, { tasks[i]}));
	}

	for (uint32_t i = 0; i < taskNum; ++i)
	{
		tasks2[i]->WaitForCompletion();
	}

	GetTaskGraphManager()->UnInit();
	GetAsyncTaskManager()->UnInit();
	return 0;
}