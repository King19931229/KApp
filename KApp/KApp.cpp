#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Publish/KObjectPool.h"

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
#include "Publish/KDump.h"

#include "Interface/IKCodec.h"
#include "Interface/IKMemory.h"
IKLogPtr pLog;

class Func : public IKTaskUnit
{
	int m_ID;
public:
	Func(int id):m_ID(id) {}
	virtual bool AsyncLoad()
	{
		KHashString str = GetHashString("AsyncLoad %d", m_ID);
		IKCodecPtr pCodec = GetCodec("D:/BIG.JPG");
		KCodecResult res;
		pCodec->Codec("D:/BIG.JPG", true, res);
		KLOG(pLog, "%s", str);
		return true;
	}
	virtual bool SyncLoad() { KLOGE(pLog, GetHashString("SyncLoad %d", m_ID));return false; }
	virtual bool HasSyncLoad() const { return false; }
};

struct Object
{
	int mData[1000];
	Object()
	{
		//printf("Object\n");
	}
	~Object()
	{
		//printf("~Object\n");
	}
};

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();
	KObjectPool<Object> pool;
	pool.Init(100);

	std::vector<Object*> datas;
	size_t uAllocCount = 100000;

	KTimer timer;
	timer.Reset();
	for(unsigned int i = 0; i < uAllocCount; ++i)
	{
		datas.push_back(new Object);
	}
	for(unsigned int i = 0; i < uAllocCount; ++i)
	{
		Object* pData = *datas.rbegin();
		datas.pop_back();
		delete pData;
	}
	printf("NOPOOL: %f\n",timer.GetMilliseconds());

	timer.Reset();
	for(size_t i = 0; i < uAllocCount; ++i)
	{
		datas.push_back(pool.Alloc());
	}

	for(unsigned int i = 0; i < uAllocCount; ++i)
	{
		Object* pData = *datas.rbegin();
		datas.pop_back();
		pool.Free(pData);
	}
	printf("POOL: %f\n",timer.GetMilliseconds());

	pool.UnInit();
	/*
	IKMemoryAllocatorPtr pAlloc = CreateAllocator();

	for(int i = 0; i < 10; ++i)
	{
		void* pRes = pAlloc->Alloc(i, 16);
		pAlloc->Free(pRes);
	}
	*/
	/*
	KObjectPool<Object> pool;
	
	std::vector<Object*> datas;
	KTimer timer;

	unsigned uAllocCount = 10000;
	pool.Init(uAllocCount);
	
	timer.Reset();
	for(unsigned int i = 0; i < uAllocCount; ++i)
	{
		datas.push_back(pool.Alloc());
	}
	for(unsigned int i = 0; i < uAllocCount; ++i)
	{
		Object* pData = *datas.rbegin();
		datas.pop_back();
		pool.Free(pData);
	}
	printf("POOL: %f %f\n", timer.GetSeconds(), timer.GetMilliseconds());
	pool.Shrink_to_fit();
	pool.UnInit();

	timer.Reset();
	for(unsigned int i = 0; i < uAllocCount; ++i)
	{
		datas.push_back(new Object);
	}
	for(unsigned int i = 0; i < uAllocCount; ++i)
	{
		Object* pData = *datas.rbegin();
		datas.pop_back();
		delete pData;
	}
	printf("NOPOOL: %f %f\n", timer.GetSeconds(), timer.GetMilliseconds());
	*/
	
#if 0
	InitCodecManager();

	pLog = CreateLog();
	pLog->Init("D:/LOG.TXT", true, true, ILM_WINDOWS);
	KTaskExecutor<true> Exc;
	Exc.PushWorkerThreads(std::thread::hardware_concurrency());
	KTimer timer;
	
	int nTaskCount = 10;

	/*{
		timer.Reset();
		std::vector<KTaskUnitProcessorPtr> ps;

		for(int i = 0; i < nTaskCount; ++i)
		{
			KTaskUnitPtr pUnit(new Func(i));
			ps.push_back(Exc.Submit(pUnit));
		}

		for(int i = 0; i < nTaskCount; ++i)
		{
			ps[i]->WaitAsync();
		}
		ps.clear();

		printf("%f %f\n", timer.GetSeconds(), timer.GetMilliseconds());
	}*/

	{
		timer.Reset();

		KTaskUnitProcessorGroupPtr pGroup = Exc.CreateGroup();
		for(int i = 0; i < nTaskCount; ++i)
		{
			KTaskUnitPtr pUnit(new Func(i));
			Exc.Submit(pGroup, pUnit);
		}
		pGroup->WaitAsync();
		printf("%f %f\n", timer.GetSeconds(), timer.GetMilliseconds());

		for(int i = 0; i < nTaskCount; ++i)
		{
			KTaskUnitPtr pUnit(new Func(i));
			Exc.Submit(pGroup, pUnit);
		}
		pGroup->WaitAsync();

		printf("%f %f\n", timer.GetSeconds(), timer.GetMilliseconds());
	}

	Exc.PopWorkerThreads(std::thread::hardware_concurrency());

	UnInitCodecManager();
#endif
}