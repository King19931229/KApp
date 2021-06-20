#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKRenderScene.h"
#include "KRender/Interface/IKGizmo.h"
#include "KRender/Interface/IKCameraController.h"
#include "KRender/Interface/IKRenderDispatcher.h"
#include "KRender/Interface/IKRayTrace.h"
#include "KRender/Publish/KCamera.h"

typedef std::function<void()> KRenderCoreInitCallback;

struct IKRenderCore
{
	virtual ~IKRenderCore() {}

	virtual bool Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window) = 0;
	virtual bool UnInit() = 0;

	// virtual bool Loop() = 0;
	virtual bool TickShouldEnd() = 0;
	virtual bool Tick() = 0;
	virtual bool Wait() = 0;

	virtual bool RegisterSecordaryWindow(IKRenderWindowPtr& window) = 0;
	virtual bool UnRegisterSecordaryWindow(IKRenderWindowPtr& window) = 0;

	virtual bool RegisterInitCallback(KRenderCoreInitCallback* callback) = 0;
	virtual bool UnRegisterInitCallback(KRenderCoreInitCallback* callback) = 0;
	virtual bool UnRegistertAllInitCallback() = 0;

	// 获取RayTrace
	virtual IKRayTraceManager* GetRayTraceMgr() = 0;
	// 获取主场景
	virtual IKRenderScene* GetRenderScene() = 0;
	virtual IKRenderDispatcher* GetRenderDispatcher() = 0;
	virtual IKRenderWindow* GetRenderWindow() = 0;
	virtual IKRenderDevice* GetRenderDevice() = 0;

	virtual IKGizmoPtr GetGizmo() = 0;
	virtual KCamera* GetCamera() = 0;
	virtual IKCameraMoveController* GetCameraController() = 0;
};
typedef std::unique_ptr<IKRenderCore> IKRenderCorePtr;

EXPORT_DLL IKRenderCorePtr CreateRenderCore(); 