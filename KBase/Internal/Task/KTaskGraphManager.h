#pragma once
#include "KBase/Interface/Task/IKTaskGraph.h"
#include "KBase/Publish/KRunableThread.h"
#include "KTaskGraphExecute.h"
#include "Internal/Task/KTaskGraphQueue.h"

class KTaskGraphManager : public IKTaskGraphManager
{
protected:
	KGraphTaskQueue m_ThreadPriorityTaskQueue[TASK_THREAD_PRIORITY_NUM];
	std::vector<KTaskGraphExecutePtr> m_ThreadGraphExecute;
	std::vector<KRunableThreadPtr> m_TaskThread;
	uint32_t m_TaskThreadNumPerPriority;
	uint32_t m_TaskThreadNum;

	const char* TaskThreadPriorityName(TaskThreadPriority priority)
	{
		switch (priority)
		{
			case TASK_THREAD_PRIORITY_LOW:
				return "Low";
			case TASK_THREAD_PRIORITY_NORMAL:
				return "Normal";
			case TASK_THREAD_PRIORITY_HIGH:
				return "High";
			case TASK_THREAD_PRIORITY_NUM:
			default:
				return "";
		}
	}
public:
	KTaskGraphManager();
	~KTaskGraphManager();

	virtual void Init() override;
	virtual void UnInit() override;
	virtual IKGraphTaskRef CreateAndDispatch(IKTaskWorkPtr work, NamedThread::Type thread, const std::vector<IKGraphTaskRef>& prerequisites) override;
	virtual IKGraphTaskRef CreateAndHold(IKTaskWorkPtr work, NamedThread::Type thread, const std::vector<IKGraphTaskRef>& prerequisites) override;
	virtual void AddTask(IKGraphTask* task, NamedThread::Type thread) override;
	virtual void AttachToThread(NamedThread::Type thread) override;
	virtual void ProcessTaskUntilIdle(NamedThread::Type thread) override;
};