#pragma once
#include "Interface/IKRenderCore.h"

class KRenderCore : public IKRenderCore
{
	IKRenderDevicePtr m_Device;
	IKRenderWindowPtr m_Window;
	bool m_bInit;
public:
	KRenderCore();
	virtual ~KRenderCore();

	virtual bool Init(RenderDevice device, size_t windowWidth, size_t windowHeight);
	virtual bool UnInit();
	virtual bool Loop();

	virtual IKRenderWindow* GetRenderWindow() { return m_Window ? m_Window.get() : nullptr; }
};