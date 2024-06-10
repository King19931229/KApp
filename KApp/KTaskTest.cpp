#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKMemory.h"
#include "KBase/Publish/KEvent.h"
#include "KBase/Interface/Task/IKAsyncTask.h"
#include "KBase/Internal/Task/KTaskThreadPool.h"
#include <functional>

KEvent event;
std::atomic_uint32_t counter;
uint32_t counter2;

void Task()
{
//	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 10));
//	counter.fetch_add(1);
	++counter2;
//	printf("counter:%d\n", counter.load());
//	printf("counter2:%d\n", counter2);
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
	
	uint32_t taskNum = 10000;
	std::vector<IKAsyncTaskPtr> tasks;
	for(uint32_t i = 0; i < taskNum; ++i)
	{
		tasks.push_back(GetAsyncTaskManager()->CreateAsyncTask(IKTaskWorkPtr(new KLambdaTaskWork(Task))));
	}
	for (uint32_t i = 0; i < taskNum; ++i)
	{
		tasks[i]->StartAsync();
	}
	GetAsyncTaskManager()->UnInit();
	return 0;
}