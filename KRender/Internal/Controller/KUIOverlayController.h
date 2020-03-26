#pragma once
#include "Interface/IKRenderWindow.h"
#include "Interface/IKUIOverlay.h"

class KUIOverlayController
{
protected:
	IKUIOverlayPtr m_UIOverlay;
	IKRenderWindow* m_Window;

	KMouseCallbackType m_MouseCallback;
	KTouchCallbackType m_TouchCallback;

	bool m_Enable;
public:
	KUIOverlayController();
	~KUIOverlayController();

	void SetEnable(bool enable) { m_Enable = enable; }

	bool Init(IKUIOverlayPtr ui, IKRenderWindow* window);
	bool UnInit();

	bool Update(float dt);
};