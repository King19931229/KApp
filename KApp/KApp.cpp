#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"

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

#include "Interface/IKLog.h"
#include "Publish/KHashString.h"
#include "Interface/IKCodec.h"
IKLogPtr pLog;

class Func : public KTaskUnit
{
	int m_ID;
public:
	Func(int id):m_ID(id) {}
	virtual bool AsyncLoad()
	{
		KHashString str = GetHashString("AsyncLoad %d", m_ID);
		IKCodecPtr pCodec = GetCodec("D:/test.JPG");
		KCodecResult res = pCodec->Codec("D:/BIG.JPG");
		KLOG(pLog, "%s", str);
		return true;
	}
	virtual bool SyncLoad() { KLOGE(pLog, GetHashString("SyncLoad %d", m_ID));return false; }
	virtual bool HasSyncLoad() const { return false; }
};

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();
	InitCodecManager();

	pLog = CreateLog();
	pLog->Init("D:/LOG.TXT", true, true, ILM_WINDOWS);
	KTaskExecutor Exc;
	Exc.PushWorkerThreads(std::thread::hardware_concurrency());
	KTimer timer;

	timer.Reset();
	std::vector<KTaskUnitProcessorPtr> ps;
	int nTaskCount = 1000;
	for(int i = 0; i < nTaskCount; ++i)
	{
		KTaskUnitPtr pUnit(new Func(i));
		KTaskUnitProcessorPtr pProcess(new KTaskUnitProcessor(pUnit));
		ps.push_back(pProcess);
		Exc.Submit(pProcess);
	}

	for(int i = 0; i < nTaskCount; ++i)
	{
		ps[i]->WaitAsync();
	}

	printf("%f %f\n", timer.GetSeconds(), timer.GetMilliseconds());
	Exc.PopWorkerThreads(std::thread::hardware_concurrency());

	UnInitCodecManager();
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