#include "Internal/KLockFreeQueue.h"
#include "Internal/KLockQueue.h"
#include "Internal/KThreadPool.h"
#include "Internal/KTimer.h"
#include "Internal/KSemaphore.h"
#include "Internal/KTaskExecutor.h"

std::atomic_int a;
bool Test(int nCount)
{
	for(int i = 0; i < nCount; ++i)
	{
		double f = tan(30.0f) * sin(40.0f) * cos(50.0f);
	}
	return 0;
}

void VoidTest()
{
	for(int i = 0; i < 1000; ++i)
	{
		double f = tan(30.0f) * sin(40.0f) * cos(50.0f);
	}
	puts("VoidTest");
}

class Func : public KTaskUnit
{
	int m_ID;
public:
	Func(int id):m_ID(id) {}
	virtual bool AsyncLoad() { printf("AsyncLoad %d\n", m_ID);return true; }
	virtual bool SyncLoad() { printf("SyncLoad %d\n", m_ID);return false; }
	virtual bool HasSyncLoad() const { return true; }
	virtual bool OnFail() { printf("OnFail %d\n", m_ID);return true; }
};

int main()
{
	KTaskExecutor Exc;
	Exc.PushWorkerThreads(4);

	std::vector<KTaskUnitProcessorPtr> ps;
	for(int i = 0; i < 100; ++i)
	{
		KTaskUnitPtr pUnit(new Func(i));
		KTaskUnitProcessorPtr pProcess(new KTaskUnitProcessor(pUnit));
		ps.push_back(pProcess);
		Exc.Submit(pProcess);
	}

	for(int i = 0; i < 100; ++i)
	{
		if(ps[i]->Cancel(true))
			puts("SUC CEL WAIT");
	}
	
	while(!Exc.AllTaskDone())
	{
		Exc.ProcessSyncTask();
	}
	
	//std::function<bool()> func = std::bind(Test, 10);
	//func();
	
	//TestClass t;
	//std::mem_fun(&TestClass::TestFunc);

	//const int TEST_COUNT = 1;
	//const int TASK_COUNT = 4000;

	/*for(int i = std::thread::hardware_concurrency() - 1; i <= std::thread::hardware_concurrency() - 1; ++i)
	{
		double fTime = 0.0f;

		for(int j = 0; j < TEST_COUNT; ++j)
		{
			KThreadPool<std::function<void(bool)>, false> pools;
			pools.PushWorkerThreads(i);
			KTimer timer;
			timer.Start();
			for(int i = 0; i < TASK_COUNT; ++i)
				pools.SubmitTask(Test(10));
			while(!pools.AllTaskDone())
			{
				pools.ProcessSyncTask();
			}
			timer.Stop();
			fTime += timer.GetDuration();
		}
		fTime /= (double)TEST_COUNT;
		printf("%s Thread Num :%d Time :%f\n", "LOCK", i, fTime);
	}*/
	/*
	for(int i = std::thread::hardware_concurrency() - 1; i <= std::thread::hardware_concurrency() - 1; ++i)
	{
		double fTime = 0.0f;
		for(int j = 0; j < TEST_COUNT; ++j)
		{
			KThreadPool<std::function<void()>, true> pools;
			pools.PushWorkerThreads(i);
			KTimer timer;
			timer.Start();
			for(int i = 0; i < TASK_COUNT; ++i)
				pools.SubmitTask(Test, Test);
			while(!pools.AllTaskDone())
			{
				pools.ProcessSyncTask();
			}
			timer.Stop();
			fTime += timer.GetDuration();
		}
		fTime /= (double)TEST_COUNT;
		printf("%s Thread Num :%d Time :%f\n", "NOLOCK", i, fTime);
	}*/
}