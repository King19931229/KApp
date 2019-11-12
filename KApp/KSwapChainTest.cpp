#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Publish/KObjectPool.h"
#include "KBase/Publish/KSystem.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KStringUtil.h"

#include "Interface/IKLog.h"
#include "Publish/KHashString.h"
#include "Publish/KDump.h"

#include "Interface/IKCodec.h"
#include "Interface/IKMemory.h"
IKLogPtr pLog;

#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderDevice.h"
#include "KRender/Interface/IKShader.h"
#include "KRender/Interface/IKProgram.h"

#include <algorithm>
#include <process.h>

class SwapChainApp
{
	bool m_bVSync;
	size_t m_nSwapChainNum;
	size_t m_DrawFrameTime;	
	size_t m_nMaxDrawFrame;

	size_t m_nPresentIdx;
	size_t m_nDrawIdx;

	size_t m_nPresentFrame;
	size_t m_nDrawFrame;

	std::vector<KSemaphorePtr> m_presentDoneSem;
	std::vector<KSemaphorePtr> m_drawDoneSem;

	std::mutex m_vsyncLock;
	KSemaphorePtr m_vsyncSem;

	std::thread m_presentThread;
	std::thread m_drawThread;
	std::thread m_vsyncThread;

	KTimer m_presentTimer;
	KTimer m_drawTimer;

public:
	SwapChainApp(bool vsync, size_t nSwapChainNum, size_t drawFrameTime, size_t nFrameNum)
		: m_bVSync(vsync),
		m_nSwapChainNum(nSwapChainNum),
		m_DrawFrameTime(drawFrameTime),
		m_nMaxDrawFrame(nFrameNum),
		m_nPresentIdx(0),
		m_nDrawIdx(1),
		m_nPresentFrame(0),
		m_nDrawFrame(0),
		m_vsyncSem(nullptr)
	{
		for(size_t i = 0; i < nSwapChainNum; ++i)
		{
			m_presentDoneSem.push_back(KSemaphorePtr(new KSemaphore()));
			m_drawDoneSem.push_back(KSemaphorePtr(new KSemaphore()));

			if(i != m_nDrawIdx)
			{
				m_presentDoneSem[i]->Notify();
			}
		}

		m_vsyncThread = std::thread([this]()
		{
			while(true)
			{
				if(m_nDrawFrame >= m_nMaxDrawFrame)
				{
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
				//printf("[vsync]\n");

				std::lock_guard<decltype(m_vsyncLock)> guard(m_vsyncLock);
				if(m_vsyncSem)
				{
					m_vsyncSem->Notify();
				}
			};
		});

		m_presentThread = std::thread([this]() 
		{
			m_presentTimer.Reset();

			while(true)
			{
				size_t nNextPrensentIdx = (m_nPresentIdx + 1) % m_nSwapChainNum;
				m_nPresentFrame++;

				if(m_nDrawFrame >= m_nMaxDrawFrame)
				{
					break;
				}

				m_drawDoneSem[nNextPrensentIdx]->Wait();

				m_nPresentIdx = nNextPrensentIdx;
				//printf("[%d] image on swap chain presenting...\n", m_nPresentIdx);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				if(m_bVSync)
				{
					{
						std::lock_guard<decltype(m_vsyncLock)> guard(m_vsyncLock);
						m_vsyncSem = KSemaphorePtr(new KSemaphore()); 
					}

					m_vsyncSem->Wait();

					{
						std::lock_guard<decltype(m_vsyncLock)> guard(m_vsyncLock);
						m_vsyncSem = nullptr;
					}
					//printf("vsync singal [%d] image on swap chain. ready to present\n", m_nPresentIdx);
				}
				//printf("[%d] image on swap chain presented\n", m_nPresentIdx);
				m_presentDoneSem[m_nPresentIdx]->Notify();

			}
			float frametime = m_presentTimer.GetMilliseconds() / (float) m_nPresentFrame;
			//printf("[present fps] %f\n", 1000.0 / frametime);
		});

		m_drawThread = std::thread([this]() 
		{
			m_drawTimer.Reset();
			while(true)
			{
				if(m_nDrawFrame >= m_nMaxDrawFrame)
				{
					break;
				}

				//printf("[%d] image on swap chain drawing...\n", m_nDrawIdx);
				std::this_thread::sleep_for(std::chrono::milliseconds(m_DrawFrameTime));
				//printf("[%d] image on swap chain drawed\n", m_nDrawIdx);
				m_nDrawFrame += 1;
				m_drawDoneSem[m_nDrawIdx]->Notify();
				size_t nNextDrawIdx = (m_nDrawIdx + 1) % m_nSwapChainNum;

				m_presentDoneSem[nNextDrawIdx]->Wait();

				m_nDrawIdx = nNextDrawIdx;
			}
			float frametime = m_drawTimer.GetMilliseconds() / (float) m_nDrawFrame;
			printf("[draw fps] %f\n", 1000.0 / frametime);
		});
	}

	bool Wait()
	{
		//m_presentThread.join();
		m_drawThread.join();
		//m_vsyncThread.join();
		return true;
	}
};

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();
	// SwapChainApp(vsync,swapchain,frametime)
	// ��ʹ�ô�ֱͬ�� ��ֻʹ��˫��������� ��20msΪ��Ⱦˢ��ʱ�� �����Ϊ��Ⱦ֡�ʻ���50fps
	// Ȼ��ʵ������ֻ�ﵽ��40fps
	// ʹ���������ܹ��˷������� ʹ��֡�ʴﵽ50fps
	//https://www.reddit.com/r/buildapc/comments/1544hx/explaining_vsync_and_other_things/
	//https://hardforum.com/threads/how-vsync-works-and-why-people-loathe-it.928593/
	{
		SwapChainApp on_2_20(true, 2, 20, 200);
		on_2_20.Wait();
	}
	{
		SwapChainApp on_3_20(true, 3, 20, 200);
		on_3_20.Wait();
	}

	// ��2msΪˢ��ʱ����Ⱦ ������ֱͬ�������֡��ֻ�ܴﵽ60fps �رմ�ֱͬ���ܹ�����500fps
	{
		SwapChainApp on_2_2(true, 2, 2, 200);
		on_2_2.Wait();
	}
	{
		SwapChainApp off_2_2(false, 2, 2, 200);
		off_2_2.Wait();
	}
}