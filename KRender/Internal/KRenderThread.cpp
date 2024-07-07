#include "KRenderThread.h"
#include "KBase/Interface/Task/IKTaskGraph.h"

KRenderThread::KRenderThread()
	: m_Stop(false)
{
}

KRenderThread::~KRenderThread()
{
}

void KRenderThread::StartUp()
{
	m_Stop = false;
}

void KRenderThread::Run()
{
	GetTaskGraphManager()->AttachToThread(NamedThread::RENDER_THREAD);
	while (!m_Stop)
	{
		GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::RENDER_THREAD);
	}
}

void KRenderThread::ShutDown()
{
	m_Stop = true;
}

KFrameSync::KFrameSync()
	: m_CurrentIdx(0)
{}

KFrameSync::~KFrameSync()
{
	if (m_Event[0])
	{
		m_Event[0]->WaitForCompletion();
	}
	if (m_Event[1])
	{
		m_Event[1]->WaitForCompletion();
	}
}

void KFrameSync::Sync()
{
	m_Event[m_CurrentIdx] = GetTaskGraphManager()->CreateAndDispatch(
		IKTaskWorkPtr(new KEmptyTaskWork()),
		NamedThread::RENDER_THREAD, {}
	);

	m_CurrentIdx = (m_CurrentIdx + 1) % 2;

	if (m_Event[m_CurrentIdx])
	{
		m_Event[m_CurrentIdx]->WaitForCompletion();
		m_Event[m_CurrentIdx].Release();
	}
}