#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSwapChain.h"
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

	IKCommandPoolPtr m_CommandPool;
	IKCommandBufferPtr m_PrimaryBuffer;

	bool m_DisplayCameraCube;
	bool m_CameraOutdate;

	bool UpdateCamera();
	bool UpdateGlobal();
public:
	KRenderer();
	~KRenderer();

	bool Init(const KCamera* camera, IKCameraCubePtr cameraCube, uint32_t width, uint32_t height);
	bool UnInit();

	IKRenderScene* GetScene() override { return m_Scene; }
	const KCamera* GetCamera() override { return m_Camera; }

	bool SetCameraCubeDisplay(bool display) override;
	bool SetSwapChain(IKSwapChain* swapChain, IKUIOverlay* uiOverlay);
	bool SetSceneCamera(IKRenderScene* scene, const KCamera* camera) override;
	bool SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback) override;
	bool RemoveCallback(IKRenderWindow* window) override;

	bool Update();
	bool Render(uint32_t chainImageIndex);
	bool Execute(uint32_t chainImageIndex);

	void OnSwapChainRecreate(uint32_t width, uint32_t height);

	inline IKCommandBufferPtr GetPrimaryCommandBuffer() { return m_PrimaryBuffer; }
};