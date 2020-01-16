#pragma once
#include "Interface/IKRenderCore.h"
#include "KRender/Internal/KDebugConsole.h"

class KRenderCore : public IKRenderCore
{
	IKRenderDevice* m_Device;
	IKRenderWindow* m_Window;
	KDebugConsole* m_DebugConsole;
	bool m_bInit;
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