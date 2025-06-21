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
#include <chrono>

#define STD_TIME_POINT_NOW() std::chrono::high_resolution_clock::now()
#define STD_TIME_POINT_UNDEFINED() std::chrono::high_resolution_clock::time_point()
#define STD_TIME_POINT_SPAN(type, span) std::chrono::duration_cast<std::chrono::duration<type>>(span).count()
#define STD_TIME_POINT_TYPE std::chrono::high_resolution_clock::time_point

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

	KDeviceInitCallback m_DeviceInitCallback;
	KDeviceUnInitCallback m_DeviceUnInitCallback;

	IKRenderer::OnWindowRenderCallback m_MainWindowRenderCB;

	IKRenderWindow* m_MainWindow;
	std::vector<IKRenderWindow*> m_SecordaryWindows;

	typedef std::unordered_set<KRenderCoreInitCallback*> InitCallbackSet;
	InitCallbackSet m_InitCallbacks;

	typedef std::unordered_set<KRenderCoreUIRenderCallback*> UICallbackSet;
	UICallbackSet m_UICallbacks;

	STD_TIME_POINT_TYPE m_LastTickTime;

	bool m_bInit;
	bool m_bTickShouldEnd;
	bool m_bSwapChainResized;

	bool InitPostProcess();
	bool UnInitPostProcess();

	bool InitGlobalManager();
	bool UnInitGlobalManager();

	bool InitRenderer();
	bool InitUISwapChain();
	bool InitController();
	bool InitGizmo();

	bool UnInitRenderer();
	bool UnInitUISwapChain();
	bool UnInitController();
	bool UnInitGizmo();

	bool InitRenderResource();
	bool UnInitRenderResource();

	bool UpdateFrameTime();
	bool UpdateUIOverlay();
	bool UpdateController();
	bool UpdateGizmo();

	void Update();
	void Render();

	// 为了敏捷开发插入的测试代码
	void DebugCode();
public:
	KRenderCore();
	virtual ~KRenderCore();

	virtual bool Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window, KRenderCoreWindowInit windowInit);
	virtual bool UnInit();
	virtual bool IsInit() const;

	virtual bool StartRenderThread();
	virtual bool EndRenderThread();

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