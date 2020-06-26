#pragma once
#include "Interface/IKEngine.h"

#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderDevice.h"
#include "KRender/Interface/IKRenderCore.h"

#include "Internal/KRenderDocCapture.h"

class KEngine : public IKEngine
{
protected:
	IKRenderWindowPtr m_Window;
	IKRenderDevicePtr m_Device;
	IKRenderCorePtr m_RenderCore;
	IKScenePtr m_Scene;
	KRenderDocCapture m_RenderDoc;
	std::unordered_map<IKRenderWindowPtr, IKSwapChainPtr> m_SecordaryWindow;

	bool m_bInit;
public:
	KEngine();
	virtual ~KEngine();

	virtual bool Init(IKRenderWindowPtr window, const KEngineOptions& options);
	virtual bool UnInit();

	virtual bool RegisterSecordaryWindow(IKRenderWindowPtr window);
	virtual bool UnRegisterSecordaryWindow(IKRenderWindowPtr window);

	virtual bool Loop();
	virtual bool Tick();

	virtual bool Wait();

	virtual IKRenderCore* GetRenderCore();
	virtual IKScene* GetScene();
};