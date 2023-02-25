#pragma once
#include "Interface/IKRenderWindow.h"
#include "Interface/IKUIOverlay.h"

class KUIOverlayController
{
protected:
	IKUIOverlay* m_UIOverlay;
	IKRenderWindow* m_Window;

	KMouseCallbackType m_MouseCallback;
	KScrollCallbackType m_ScrollCallback;
	KTouchCallbackType m_TouchCallback;

	bool m_Enable;
public:
	KUIOverlayController();
	~KUIOverlayController();

	void SetEnable(bool enable) { m_Enable = enable; }

	bool Init(IKUIOverlay* ui, IKRenderWindow* window);
	bool UnInit();

	bool Update(float dt);
};