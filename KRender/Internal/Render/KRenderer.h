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
#include "KMeshLODView.h"

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
	struct LODView
	{
		IKRenderScene* scene = nullptr;
		const KCamera* camera = nullptr;
		KMeshLODView selection;
	};
	std::unordered_map<IKRenderWindow*, LODView> m_LODViews;
	bool m_MainUsesGPUScene = false;
	
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
		void SetThreadNum(uint32_t threadNum);
	};

	GPUQueueMiscs m_Shadow;
	GPUQueueMiscs m_PreGraphics;
	GPUQueueMiscs m_ParallelGraphics;
	GPUQueueMiscs m_PostGraphics;
	GPUQueueMiscs m_Compute;

	KRenderJobExecuteThreadPool m_RenderJobExecuteThreadPool;

	KRunableThreadPtr m_RHIThread;

	int m_PrevMultithreadCount;
	int m_MultithreadCount;

	bool m_PrevEnableAsyncCompute;
	int m_PrevPeelingMethod;

	bool m_DisplayCameraCube;
	bool m_CameraOutdate;

	bool UpdateCamera();
	bool UpdateGlobal();
	void ResizePeelingResources(uint32_t width, uint32_t height);

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

	int& GetMultithreadCount() { return m_MultithreadCount; }
	KMeshLODSelectionStats GetLODStats() const;
	bool MainUsesGPUScene() const { return m_MainUsesGPUScene; }

	KRHICommandList& GetRHICommandList() { return m_RHICommandList; }

	bool Update();
	void ReleaseFrameResources(uint32_t frameNum);
	void RemoveUnreferenced();
	bool Render(uint32_t chainImageIndex);
	bool Execute(uint32_t chainImageIndex);

	void OnSwapChainRecreate(uint32_t width, uint32_t height);
};
