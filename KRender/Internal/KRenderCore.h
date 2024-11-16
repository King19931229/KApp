#pragma once
#include "Interface/IKRenderCore.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderer.h"
#include "Interface/IKSwapChain.h"

#include "KRender/Internal/KDebugConsole.h"

#include "Internal/Controller/KCameraMoveController.h"
#include "Internal/Controller/KUIOverlayController.h"
#include "Internal/Controller/KGizmoController.h"

#include "KBase/Publish/KRunableThread.h"
#include "KRenderThread.h"

#include <unordered_set>

class KRenderCore : public IKRenderCore
{
protected:
	IKRenderDevice* m_Device;
	KDebugConsole* m_DebugConsole;
	KCamera m_Camera;

	KRunableThreadPtr m_RenderThread;
	KFrameSync m_FrameSync;

	IKGizmoPtr m_Gizmo;
	IKCameraCubePtr m_CameraCube;

	KCameraMoveController m_CameraMoveController;
	KUIOverlayController m_UIController;
	KGizmoController m_GizmoContoller;

	KKeyboardCallbackType m_KeyCallback;

	KDeviceInitCallback m_InitCallback;
	KDeviceUnInitCallback m_UnitCallback;

	IKRenderer::OnWindowRenderCallback m_MainWindowRenderCB;

	IKRenderWindow* m_MainWindow;
	std::unordered_map<IKRenderWindow*, IKSwapChainPtr> m_SecordaryWindow;

	typedef std::unordered_set<KRenderCoreInitCallback*> InitCallbackSet;
	InitCallbackSet m_InitCallbacks;

	typedef std::unordered_set<KRenderCoreUIRenderCallback*> UICallbackSet;
	UICallbackSet m_UICallbacks;

	bool m_bInit;
	bool m_bTickShouldEnd;
	bool m_bSwapChainResized;

	bool InitPostProcess();
	bool UnInitPostProcess();

	bool InitGlobalManager();
	bool UnInitGlobalManager();

	bool InitRenderer();
	bool InitController();
	bool InitGizmo();

	bool UnInitRenderer();
	bool UnInitController();
	bool UnInitGizmo();

	bool InitRenderResource();
	bool UnInitRenderResource();

	bool UpdateFrameTime();
	bool UpdateUIOverlay();
	bool UpdateController();
	bool UpdateGizmo();

	void OnPostPresent(uint32_t chainIndex, uint32_t frameIndex);

	void Update();
	void Render();

	// 为了敏捷开发插入的测试代码
	void DebugCode();
public:
	KRenderCore();
	virtual ~KRenderCore();

	virtual bool Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window);
	virtual bool UnInit();
	virtual bool IsInit() const;

	virtual bool Loop();
	virtual bool TickShouldEnd();
	virtual bool Tick();
	virtual bool Wait();

	virtual bool RegisterSecordaryWindow(IKRenderWindowPtr& window);
	virtual bool UnRegisterSecordaryWindow(IKRenderWindowPtr& window);

	virtual bool RegisterInitCallback(KRenderCoreInitCallback* callback);
	virtual bool UnRegisterInitCallback(KRenderCoreInitCallback* callback);
	virtual bool UnRegistertAllInitCallback();

	virtual bool RegisterUIRenderCallback(KRenderCoreUIRenderCallback* callback);
	virtual bool UnRegisterUIRenderCallback(KRenderCoreUIRenderCallback* callback);
	virtual bool UnRegistertAllUIRenderCallback();

	virtual IKRenderScene* GetRenderScene();
	virtual IKRenderer* GetRenderer();

	virtual IKRenderWindow* GetMainWindow() { return m_MainWindow; }
	virtual IKRenderDevice* GetRenderDevice() { return m_Device; }

	virtual IKGizmoPtr GetGizmo() { return m_Gizmo; }
	virtual KCamera* GetCamera() { return &m_Camera; }
	virtual IKCameraMoveController* GetCameraController() { return &m_CameraMoveController; }
};