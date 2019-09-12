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

	std::thread m_presentThread;
	std::thread m_drawThread;

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
		m_nDrawFrame(0)
	{
		for(size_t i = 0; i < nSwapChainNum; ++i)
		{
			m_presentDoneSem.push_back(KSemaphorePtr(new KSemaphore()));
			m_drawDoneSem.push_back(KSemaphorePtr(new KSemaphore()));
			if(m_bVSync)
			{
				if(i != m_nDrawIdx)
				{
					m_presentDoneSem[i]->Notify();
				}
			}
		}

		m_presentThread = std::thread([this]() 
		{
			m_presentTimer.Reset();

			while(true)
			{
				size_t nNextPrensentIdx = (m_nPresentIdx + 1) % m_nSwapChainNum;
				m_nPresentFrame++;
				if(m_drawDoneSem[nNextPrensentIdx]->TryWait(0))
				{
					m_nPresentIdx = nNextPrensentIdx;
					//printf("[%d] image on swap chain presenting...\n", m_nPresentIdx);
					std::this_thread::sleep_for(std::chrono::milliseconds(16));
					//printf("[%d] image on swap chain presented\n", m_nPresentIdx);
					if(m_bVSync)
					{
						m_presentDoneSem[m_nPresentIdx]->Notify();
					}
					if(m_nDrawFrame >= m_nMaxDrawFrame)
					{
						break;
					}
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(16));
					//printf("[%d] image on swap chain is not ready for presented yet...\n", nNextPrensentIdx);
				}
			}
			float frametime = m_presentTimer.GetMilliseconds() / (float) m_nPresentFrame;
			//printf("[present fps] %f\n", 1000.0 / frametime);
		});


		m_drawThread = std::thread([this]() 
		{
			m_drawTimer.Reset();
			while(true)
			{
				//printf("[%d] image on swap chain drawing...\n", m_nDrawIdx);
				std::this_thread::sleep_for(std::chrono::milliseconds(m_DrawFrameTime));
				//printf("[%d] image on swap chain drawed\n", m_nDrawIdx);
				m_nDrawFrame += 1;
				m_drawDoneSem[m_nDrawIdx]->Notify();
				size_t nNextDrawIdx = (m_nDrawIdx + 1) % m_nSwapChainNum;
				if(m_bVSync)
				{
					m_presentDoneSem[nNextDrawIdx]->Wait();
				}
				m_nDrawIdx = nNextDrawIdx;
				if(m_nDrawFrame >= m_nMaxDrawFrame)
				{
					break;
				}
			}
			float frametime = m_drawTimer.GetMilliseconds() / (float) m_nDrawFrame;
			printf("[draw fps] %f\n", 1000.0 / frametime);
		});
	}

	bool Wait()
	{
		m_presentThread.join();
		m_drawThread.join();
		return true;
	}
};

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();
	// SwapChainApp(vsync,swapchain,frametime)
	// 在使用垂直同步 并只使用双缓冲情况下 以20ms为渲染刷新时间 你会认为渲染帧率会有50fps
	// 然而实际上你只达到了40fps
	// 使用三缓冲能够克服此现象 使得帧率达到50fps
	//https://www.reddit.com/r/buildapc/comments/1544hx/explaining_vsync_and_other_things/
	//https://hardforum.com/threads/how-vsync-works-and-why-people-loathe-it.928593/
	{
		SwapChainApp on_2_20(true, 2, 20, 500);
		on_2_20.Wait();
	}
	{
		SwapChainApp on_3_20(true, 3, 20, 500);
		on_3_20.Wait();
	}

	// 以2ms为刷新时间渲染 开启垂直同步下最高帧率只能达到60fps 关闭垂直同步能够到达500fps
	{
		SwapChainApp on_2_2(true, 2, 2, 500);
		on_2_2.Wait();
	}
	{
		SwapChainApp off_2_2(false, 2, 2, 500);
		off_2_2.Wait();
	}
}