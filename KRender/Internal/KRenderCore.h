#pragma once
#include "Interface/IKRenderCore.h"
#include "Interface/IKRenderDevice.h"

#include "KRender/Internal/KDebugConsole.h"

#include "Internal/Controller/KCameraMoveController.h"
#include "Internal/Controller/KUIOverlayController.h"
#include "Internal/Controller/KGizmoController.h"

class KRenderCore : public IKRenderCore
{
protected:
	IKRenderDevice* m_Device;
	IKRenderWindow* m_Window;
	KDebugConsole* m_DebugConsole;
	KCamera m_Camera;

	IKGizmoPtr m_Gizmo;

	KCameraMoveController m_CameraMoveController;
	KUIOverlayController m_UIController;
	KGizmoController m_GizmoContoller;

	KKeyboardCallbackType m_KeyCallback;
	KDevicePresentCallback m_PresentCallback;

	bool m_MultiThreadSumbit;
	bool m_OctreeDebugDraw;
	bool m_MouseCtrlCamera;

	bool m_bInit;

	bool InitRenderDispatcher();
	bool InitController();
	bool InitGizmo();

	bool UnInitRenderDispatcher();
	bool UnInitController();
	bool UnInitGizmo();

	bool UpdateFrameTime();
	bool UpdateCamera(size_t frameIndex);
	bool UpdateUIOverlay(size_t frameIndex);
	bool UpdateController();
	bool UpdateGizmo();

	void OnPresent(uint32_t chainIndex, uint32_t frameIndex);
public:
	KRenderCore();
	virtual ~KRenderCore();

	virtual bool Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window);
	virtual bool UnInit();
	virtual bool Loop();
	virtual bool Tick();

	virtual IKRenderWindow* GetRenderWindow() { return m_Window; }
	virtual IKRenderDevice* GetRenderDevice() { return m_Device; }
};