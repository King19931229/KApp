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
	static thread_local NamedThread::Type m_ThreadIdTLS;

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
	virtual IKGraphTaskEventRef CreateAndDispatch(IKTaskWorkPtr work, NamedThread::Type thread, const std::vector<IKGraphTaskEventRef>& prerequisites) override;
	virtual IKGraphTaskEventRef CreateAndHold(IKTaskWorkPtr work, NamedThread::Type thread, const std::vector<IKGraphTaskEventRef>& prerequisites) override;
	virtual void Dispatch(IKGraphTaskEventRef taskEvent) override;
	virtual void AddTask(IKGraphTaskRef task, NamedThread::Type thread) override;
	virtual void AttachToThread(NamedThread::Type thread) override;
	virtual NamedThread::Type GetThisThreadId() const override;
	virtual void ProcessTaskUntilIdle(NamedThread::Type thread) override;
	virtual void ProcessTaskUntilQuit(NamedThread::Type thread) override;
};