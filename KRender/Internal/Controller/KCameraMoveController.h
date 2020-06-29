#pragma once
#include "Interface/IKRenderWindow.h"
#include "Interface/IKGizmo.h"
#include "Interface/IKCameraController.h"
#include "Publish/KCamera.h"

class KCameraMoveController : public IKCameraMoveController
{
protected:
	KCamera* m_Camera;
	IKRenderWindow* m_Window;
	IKGizmoPtr m_Gizmo;

	KKeyboardCallbackType m_KeyCallback;
	KMouseCallbackType m_MouseCallback;
	KScrollCallbackType m_ScrollCallback;
	KTouchCallbackType m_TouchCallback;
	KFocusCallbackType m_FocusCallback;
	KGizmoTriggerCallback m_GizmoTriggerCallback;

	int m_Move[3];

	bool m_MouseDown[INPUT_MOUSE_BUTTON_COUNT];
	float m_MousePos[2];

	int m_TouchAction;
	int m_LastTouchCount;
	float m_LastTouchDistance;
	bool m_Touch[2];
	float m_TouchPos[2][2];

	bool m_Enable;
	bool m_GizmoTriggered;

	float m_Speed;

	void ZeroData();
	inline bool IsEnable() const { return m_Enable && !m_GizmoTriggered; }
public:
	KCameraMoveController();
	~KCameraMoveController();

	void SetEnable(bool enable) override { m_Enable = enable; }
	void SetSpeed(float speed) override  { m_Speed = (speed > 0.0f) ? speed : m_Speed; }
	bool Init(KCamera* camera, IKRenderWindow* window, IKGizmoPtr gizmo) override;
	bool UnInit() override;

	bool Update(float dt);
};