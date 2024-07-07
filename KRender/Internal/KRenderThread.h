#pragma once
#include "KBase/Interface/IKRunable.h"
#include "KBase/Interface/Task/IKTaskGraph.h"

class KRenderThread : public IKRunable
{
protected:
	bool m_Stop;
public:
	KRenderThread();
	~KRenderThread();

	virtual void StartUp() override;
	virtual void Run() override;
	virtual void ShutDown() override;
};

typedef std::function<void()> RenderTaskFunctionType;

template<typename TaskName>
inline IKGraphTaskEventRef EnqueueRenderCommand(RenderTaskFunctionType command)
{
	struct KRenderThreadTaskWork : public IKTaskWork
	{
		RenderTaskFunctionType task;

		KRenderThreadTaskWork(RenderTaskFunctionType inTask)
			: task(inTask)
		{}

		void DoWork() override
		{
			task();
		}

		void Abandon() override 
		{}

		const char* GetDebugInfo() override
		{
			return TaskName::GetName();
		}
	};

	return GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KTaskWork<KRenderThreadTaskWork>(command)), NamedThread::RENDER_THREAD, {});
}

#define ENQUEUE_RENDER_COMMAND(Name)\
	struct KRender##Name##Task\
	{\
		static const char* GetName() { return #Name; }\
	};\
	EnqueueRenderCommand<KRender##Name##Task>

#define FLUSH_RENDER_COMMAND()\
{\
	IKGraphTaskEventRef completeEvent = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KEmptyTaskWork()), NamedThread::RENDER_THREAD, {});\
	completeEvent->WaitForCompletion();\
}

class KFrameSync
{
protected:
	IKGraphTaskEventRef m_Event[2];
	uint32_t m_CurrentIdx;
public:
	KFrameSync();
	~KFrameSync();

	void Sync();
};