#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKGizmo.h"
#include "KRender/Interface/IKCameraController.h"
#include "KRender/Interface/IKRenderer.h"
#include "KRender/Interface/IKRayTrace.h"
#include "KRender/Publish/KCamera.h"

typedef std::function<void()> KRenderCoreInitCallback;
typedef std::function<void()> KRenderCoreUIRenderCallback;
typedef std::function<void(IKRenderWindowPtr& window)> KRenderCoreWindowInit;

struct IKRenderCore
{
	virtual ~IKRenderCore() {}

	virtual bool Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window, KRenderCoreWindowInit windowInit) = 0;
	virtual bool UnInit() = 0;
	virtual bool IsInit() const = 0;

	virtual bool StartRenderThread() = 0;
	virtual bool EndRenderThread() = 0;

	virtual bool TickShouldEnd() = 0;
	virtual bool Tick() = 0;
	virtual bool Wait() = 0;

	virtual bool RegisterSecordaryWindow(IKRenderWindowPtr& window) = 0;
	virtual bool UnRegisterSecordaryWindow(IKRenderWindowPtr& window) = 0;

	virtual bool RegisterInitCallback(KRenderCoreInitCallback* callback) = 0;
	virtual bool UnRegisterInitCallback(KRenderCoreInitCallback* callback) = 0;
	virtual bool UnRegistertAllInitCallback() = 0;

	virtual bool RegisterUIRenderCallback(KRenderCoreUIRenderCallback* callback) = 0;
	virtual bool UnRegisterUIRenderCallback(KRenderCoreUIRenderCallback* callback) = 0;
	virtual bool UnRegistertAllUIRenderCallback() = 0;

	// 获取主场景
	virtual IKRenderScene* GetRenderScene() = 0;
	virtual IKRenderer* GetRenderer() = 0;
	virtual IKRenderWindow* GetMainWindow() = 0;
	virtual IKRenderDevice* GetRenderDevice() = 0;

	virtual IKGizmoPtr GetGizmo() = 0;
	virtual KCamera* GetCamera() = 0;
	virtual IKCameraMoveController* GetCameraController() = 0;
};
typedef std::unique_ptr<IKRenderCore> IKRenderCorePtr;

EXPORT_DLL IKRenderCorePtr CreateRenderCore(); 