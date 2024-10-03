#include "KRHICommandList.h"
#include "Interface/IKCommandBuffer.h"

KRHIThread::KRHIThread()
	: m_Stop(false)
{
}

KRHIThread::~KRHIThread()
{
}

void KRHIThread::StartUp()
{
	m_Stop = false;
}

void KRHIThread::Run()
{
	GetTaskGraphManager()->AttachToThread(NamedThread::RHI_THREAD);
	while (!m_Stop)
	{
		GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::RHI_THREAD);
	}
}

void KRHIThread::ShutDown()
{
	m_Stop = true;
}

void KUpdateUniformBuffer::Execute(KRHICommandList& commandList)
{
	IKCommandBufferPtr commandBuf = commandList.GetCommandBuffer();
	void* pData = nullptr;
	uniformBuffer->Map(&pData);
	memcpy(POINTER_OFFSET(pData, offset), data.data(), size);
	uniformBuffer->UnMap();
}

KRHICommandList::KRHICommandList()
	: m_CommandHead(nullptr)
	, m_CommandNext(&m_CommandHead)
	, m_ImmediateMode(true)
{

}

KRHICommandList::~KRHICommandList()
{
}

void KRHICommandList::SetImmediate(bool immediate)
{
	if (immediate)
	{
		Flush(RHICommandFlush::FlushRHIThread);
	}
	m_ImmediateMode = immediate;
}

void KRHICommandList::Flush(RHICommandFlush::Type flushType)
{
	if (flushType >= RHICommandFlush::DispatchToRHIThread)
	{
		if (m_AsyncTask.Get() && !m_AsyncTask->IsCompleted())
		{
			IKGraphTaskEventRef task = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KRHICommandTaskWork(*this, m_CommandHead)),
				NamedThread::RHI_THREAD, { m_AsyncTask });
			m_AsyncTask = task;
		}
		else
		{
			m_AsyncTask = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KRHICommandTaskWork(*this, m_CommandHead)), NamedThread::RHI_THREAD, {});
		}
		m_CommandHead = nullptr;
		m_CommandNext = &m_CommandHead;
	}
	if (flushType >= RHICommandFlush::FlushRHIThread)
	{
		if (m_AsyncTask.Get())
		{
			if (!m_AsyncTask->IsCompleted())
			{
				m_AsyncTask->WaitForCompletion();
			}
			m_AsyncTask.Release();
		}
	}
}

void KRHICommandList::UpdateUniformBuffer(IKUniformBufferPtr uniformBuffer, void* data, uint32_t offset, uint32_t size)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KUpdateUniformBuffer(uniformBuffer, data, offset, size));
	if (m_ImmediateMode)
	{
		command->Execute(*this);
		return;
	}
	*m_CommandNext = command;
	m_CommandNext = &command->next;
}