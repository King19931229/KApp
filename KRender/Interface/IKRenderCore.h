#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKRenderScene.h"

struct IKRenderCore
{
	virtual ~IKRenderCore() {}

	virtual bool Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window) = 0;
	virtual bool UnInit() = 0;
	virtual bool Loop() = 0;
	virtual bool Tick() = 0;

	virtual IKRenderScene* GetRenderScene() = 0;
	virtual IKRenderWindow* GetRenderWindow() = 0;
	virtual IKRenderDevice* GetRenderDevice() = 0;
};
typedef std::unique_ptr<IKRenderCore> IKRenderCorePtr;

EXPORT_DLL IKRenderCorePtr CreateRenderCore(); 