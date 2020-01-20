#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Publish/KObjectPool.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Publish/KHashString.h"
#include "KBase/Publish/KDump.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/IKMemory.h"

class Func0 : public IKTaskUnit
{
	int m_ID;
public:
	Func0(int id): m_ID(id) {}
	virtual bool AsyncLoad()
	{
		//printf("Func0 asycn %d\n", m_ID);
		std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 5));
		return true;
	}
	virtual bool SyncLoad()
	{
		//printf("Func0 sycn %d\n", m_ID);
		return true;
	}
	virtual bool HasSyncLoad() const { return true; }
};

class Func1 : public IKTaskUnit
{
	int m_ID;
	KTaskUnitProcessorPtr m_Ptr;
public:
	Func1(KTaskUnitProcessorPtr ptr, int id) :m_Ptr(ptr), m_ID(id) {}
	virtual bool AsyncLoad()
	{
		//printf("Func1 asycn %d\n", m_ID);
		m_Ptr->Wait();
		return true;
	}
	virtual bool SyncLoad()
	{
		printf("Func1 sycn %d\n", m_ID);
		return true;
	}
	virtual bool HasSyncLoad() const { return true; }
};

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();

	int nTaskCount = 10;

	{
		KTimer timer;

		KTaskExecutor<false> Exc;
		Exc.PushWorkerThreads(std::thread::hardware_concurrency());

		std::vector<KTaskUnitProcessorPtr> allPtr0;
		std::vector<KTaskUnitProcessorPtr> allPtr1;
		allPtr0.reserve(nTaskCount);
		allPtr1.reserve(nTaskCount);
		{
			timer.Reset();

			for (int i = 0; i < nTaskCount; ++i)
			{
				KTaskUnitPtr pUnit0(new Func0(2 * i));
				KTaskUnitProcessorPtr ptr0 = Exc.Submit(pUnit0);
				allPtr0.push_back(ptr0);

				KTaskUnitPtr pUnit1(new Func1(ptr0, 2 * i + 1));
				KTaskUnitProcessorPtr ptr1 = Exc.Submit(pUnit1);
				allPtr1.push_back(ptr1);
			}

			while (!Exc.AllTaskDone())
			{
				Exc.ProcessSyncTask();
			}

			printf("%f\n", timer.GetMilliseconds());
		}
	}
}