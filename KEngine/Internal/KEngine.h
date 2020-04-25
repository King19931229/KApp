#pragma once
#include "Interface/IKEngine.h"

#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderDevice.h"
#include "KRender/Interface/IKRenderCore.h"

class KEngine : public IKEngine
{
protected:
	IKRenderWindowPtr m_Window;
	IKRenderDevicePtr m_Device;
	IKRenderCorePtr m_RenderCore;

	bool m_bInit;
public:
	KEngine();
	virtual ~KEngine();

	virtual bool Init(IKRenderWindowPtr window, const KEngineOptions& options);
	virtual bool UnInit();

	virtual bool Loop();
	virtual bool Tick();

	virtual bool Wait();

	virtual IKRenderCore* GetRenderCore();
};