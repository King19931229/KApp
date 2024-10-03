#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSwapChain.h"
#include "Interface/IKQueue.h"
#include "Interface/IKGizmo.h"
#include "Interface/IKRenderer.h"
#include "Internal/Scene/KRenderScene.h"
#include "Internal/FrameGraph/KFrameGraph.h"
#include "Internal/KRenderThreadPool.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KRunableThread.h"
#include "Publish/KCamera.h"
#include "KRHICommandList.h"

class KRenderer;

class KMainPass : public KFrameGraphPass
{
protected:
	KRenderer& m_Master;
public:
	KMainPass(KRenderer& master);
	~KMainPass();

	bool Init();
	bool UnInit();

	bool HasSideEffect() const override { return true; }

	bool Setup(KFrameGraphBuilder& builder) override;
	bool Execute(KFrameGraphExecutor& executor) override;
};
typedef std::shared_ptr<KMainPass> KMainPassPtr;

struct KRendererInitContext
{
	const KCamera* camera;
	IKCameraCubePtr cameraCube;
	uint32_t width;
	uint32_t height;
	bool enableAsyncCompute;
	bool enableMultithreadRender;

	KRendererInitContext()
		: camera(nullptr)
		, cameraCube(nullptr)
		, width(0)
		, height(0)
		, enableAsyncCompute(false)
		, enableMultithreadRender(false)
	{}
};

class KRenderer : public IKRenderer
{
	friend class KMainBasePass;
protected:
	IKSwapChain* m_SwapChain;
	IKUIOverlay* m_UIOverlay;
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	IKCameraCubePtr m_CameraCube;
	KMainPassPtr m_Pass;
	glm::mat4 m_PrevViewProj;

	RenderPassCallFuncType m_BasePassMainCallFunc;
	RenderPassCallFuncType m_BasePassPostCallFunc;
	RenderPassCallFuncType m_DebugCallFunc;
	RenderPassCallFuncType m_ForegroundCallFunc;
	std::unordered_map<IKRenderWindow*, OnWindowRenderCallback*> m_WindowRenderCB;
	
	KRHICommandList m_RHICommandList;

	struct GPUQueueMiscs
	{
		std::vector<IKCommandPoolPtr> threadPools;
		IKCommandPoolPtr pool;
		IKSemaphorePtr finish;
		IKQueuePtr queue;

		QueueCategory category;
		uint32_t queueIndex;

		GPUQueueMiscs()
			: category(QUEUE_GRAPHICS)
			, queueIndex(0)
		{
		}

		void Init(QueueCategory category, uint32_t queueIndex, uint32_t threadNum, const char* name);
		void UnInit();
		void Reset();
		void SetThreadNum(uint32_t threadNum);
	};

	GPUQueueMiscs m_Shadow;
	GPUQueueMiscs m_PreGraphics;
	GPUQueueMiscs m_PostGraphics;
	GPUQueueMiscs m_Compute;

	KRenderThreadPool m_ThreadPool;

	KRunableThreadPtr m_RHIThread;

	int m_PrevMultithreadCount;
	int m_MultithreadCount;

	bool m_PrevEnableAsyncCompute;
	bool m_EnableAsyncCompute;

	bool m_EnableMultithreadRender;

	bool m_DisplayCameraCube;
	bool m_CameraOutdate;

	bool UpdateCamera();
	bool UpdateGlobal();

	bool SwitchAsyncCompute(bool enableAsyncCompute);
	bool ResetThreadNum(uint32_t threadNum);
public:
	KRenderer();
	~KRenderer();

	bool Init(const KRendererInitContext& initContext);
	bool UnInit();

	IKRenderScene* GetScene() override { return m_Scene; }
	const KCamera* GetCamera() override { return m_Camera; }

	bool SetCameraCubeDisplay(bool display) override;
	bool SetSwapChain(IKSwapChain* swapChain);
	bool SetUIOverlay(IKUIOverlay* uiOverlay);
	bool SetSceneCamera(IKRenderScene* scene, const KCamera* camera) override;
	bool SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback) override;
	bool RemoveCallback(IKRenderWindow* window) override;

	bool& GetEnableAsyncCompute() { return m_EnableAsyncCompute; }
	bool& GetEnableMultithreadRender() { return m_EnableMultithreadRender; }
	int& GetMultithreadCount() { return m_MultithreadCount; }

	bool Update();
	bool Render(uint32_t chainImageIndex_);
	bool Execute(uint32_t chainImageIndex);

	void OnSwapChainRecreate(uint32_t width, uint32_t height);
};