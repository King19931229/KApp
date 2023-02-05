#pragma once
#include "Interface/IKRenderCore.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderer.h"

#include "KRender/Internal/KDebugConsole.h"

#include "Internal/Controller/KCameraMoveController.h"
#include "Internal/Controller/KUIOverlayController.h"
#include "Internal/Controller/KGizmoController.h"

#include <unordered_set>

class KRenderCore : public IKRenderCore
{
protected:
	IKRenderDevice* m_Device;
	IKRenderWindow* m_Window;
	KDebugConsole* m_DebugConsole;
	KCamera m_Camera;

	IKGizmoPtr m_Gizmo;
	IKCameraCubePtr m_CameraCube;

	KCameraMoveController m_CameraMoveController;
	KUIOverlayController m_UIController;
	KGizmoController m_GizmoContoller;

	KKeyboardCallbackType m_KeyCallback;

	KDevicePresentCallback m_PrePresentCallback;
	KDevicePresentCallback m_PostPresentCallback;
	KSwapChainRecreateCallback m_SwapChainCallback;
	KDeviceInitCallback m_InitCallback;
	KDeviceUnInitCallback m_UnitCallback;

	IKRenderer::OnWindowRenderCallback m_MainWindowRenderCB;

	std::unordered_map<IKRenderWindow*, IKSwapChainPtr> m_SecordaryWindow;

	typedef std::unordered_set<KRenderCoreInitCallback*> CallbackSet;
	CallbackSet m_Callbacks;

	bool m_bInit;
	bool m_bTickShouldEnd;

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

	void OnPrePresent(uint32_t chainIndex, uint32_t frameIndex);
	void OnPostPresent(uint32_t chainIndex, uint32_t frameIndex);
	void OnSwapChainRecreate(uint32_t width, uint32_t height);

	// 为了敏捷开发插入的测试代码
	void DebugCode();
public:
	KRenderCore();
	virtual ~KRenderCore();

	virtual bool Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window);
	virtual bool UnInit();

	virtual bool Loop();
	virtual bool TickShouldEnd();
	virtual bool Tick();
	virtual bool Wait();

	virtual bool RegisterSecordaryWindow(IKRenderWindowPtr& window);
	virtual bool UnRegisterSecordaryWindow(IKRenderWindowPtr& window);

	virtual bool RegisterInitCallback(KRenderCoreInitCallback* callback);
	virtual bool UnRegisterInitCallback(KRenderCoreInitCallback* callback);
	virtual bool UnRegistertAllInitCallback();

	virtual IKRayTraceManager* GetRayTraceMgr();
	virtual IKRenderScene* GetRenderScene();
	virtual IKRenderer* GetRenderer();

	virtual IKRenderWindow* GetRenderWindow() { return m_Window; }
	virtual IKRenderDevice* GetRenderDevice() { return m_Device; }

	virtual IKGizmoPtr GetGizmo() { return m_Gizmo; }
	virtual KCamera* GetCamera() { return &m_Camera; }
	virtual IKCameraMoveController* GetCameraController() { return &m_CameraMoveController; }
};