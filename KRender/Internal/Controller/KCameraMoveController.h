#pragma once
#include "Interface/IKRenderWindow.h"
#include "Publish/KCamera.h"

class KCameraMoveController
{
protected:
	KCamera* m_Camera;
	IKRenderWindow* m_Window;

	KKeyboardCallbackType m_KeyCallback;
	KMouseCallbackType m_MouseCallback;
	KScrollCallbackType m_ScrollCallback;
	KTouchCallbackType m_TouchCallback;
	KFocusCallbackType m_FocusCallback;

	int m_Move[3];

	bool m_MouseDown[INPUT_MOUSE_BUTTON_COUNT];
	float m_MousePos[2];

	int m_TouchAction;
	int m_LastTouchCount;
	float m_LastTouchDistance;
	bool m_Touch[2];
	float m_TouchPos[2][2];

	bool m_Enable;

	void ZeroData();
public:
	KCameraMoveController();
	~KCameraMoveController();

	void SetEnable(bool enable) { m_Enable = enable; }

	bool Init(KCamera* camera, IKRenderWindow* window);
	bool UnInit();

	bool Update(float dt);
};