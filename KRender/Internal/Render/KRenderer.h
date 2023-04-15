#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSwapChain.h"
#include "Interface/IKQueue.h"
#include "Interface/IKGizmo.h"
#include "Interface/IKRenderer.h"
#include "Internal/Scene/KRenderScene.h"
#include "Internal/FrameGraph/KFrameGraph.h"
#include "KBase/Publish/KThreadPool.h"
#include "Publish/KCamera.h"

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

	KRendererInitContext()
		: camera(nullptr)
		, cameraCube(nullptr)
		, width(0)
		, height(0)
		, enableAsyncCompute(true)
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

	RenderPassCallFuncType m_DebugCallFunc;
	RenderPassCallFuncType m_ForegroundCallFunc;

	std::unordered_map<IKRenderWindow*, OnWindowRenderCallback*> m_WindowRenderCB;

	struct GPUQueueMiscs
	{
		IKCommandPoolPtr pool;
		IKSemaphorePtr finish;
		IKQueuePtr queue;

		void Init(QueueCategory category, uint32_t queueIndex, const char* name);
		void UnInit();
		void Reset();
	};

	GPUQueueMiscs m_Shadow;
	GPUQueueMiscs m_PreGraphics;
	GPUQueueMiscs m_PostGraphics;
	GPUQueueMiscs m_Compute;

	bool m_PrevEnableAsyncCompute;
	bool m_EnableAsyncCompute;
	bool m_DisplayCameraCube;
	bool m_CameraOutdate;

	bool UpdateCamera();
	bool UpdateGlobal();

	bool SwitchAsyncCompute(bool enableAsyncCompute);
public:
	KRenderer();
	~KRenderer();

	bool Init(const KRendererInitContext& initContext);
	bool UnInit();

	IKRenderScene* GetScene() override { return m_Scene; }
	const KCamera* GetCamera() override { return m_Camera; }

	bool SetCameraCubeDisplay(bool display) override;
	bool SetSwapChain(IKSwapChain* swapChain, IKUIOverlay* uiOverlay);
	bool SetSceneCamera(IKRenderScene* scene, const KCamera* camera) override;
	bool SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback) override;
	bool RemoveCallback(IKRenderWindow* window) override;

	bool& GetEnableAsyncCompute() { return m_EnableAsyncCompute; }

	bool Update();
	bool Render(uint32_t chainImageIndex_);
	bool Execute(uint32_t chainImageIndex);

	void OnSwapChainRecreate(uint32_t width, uint32_t height);
};