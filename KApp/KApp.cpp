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

class Func : public IKTaskUnit
{
	int m_ID;
public:
	Func(int id):m_ID(id) {}
	virtual bool AsyncLoad()
	{
		KHashString str = GetHashString("AsyncLoad %d", m_ID);
		IKCodecPtr pCodec = GetCodec("D:/BIG.JPG");
		KCodecResult res = pCodec->Codec("D:/BIG.JPG");
		//KLOG(pLog, "%s", str);
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
	KTaskExecutor<true> Exc;
	Exc.PushWorkerThreads(std::thread::hardware_concurrency());
	KTimer timer;

	int nTaskCount = 1000;

	{
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
	}

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
	}

	Exc.PopWorkerThreads(std::thread::hardware_concurrency());

	UnInitCodecManager();
}