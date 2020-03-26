#pragma once
#include "Interface/IKRenderWindow.h"
#include "Interface/IKGizmo.h"

class KGizmoController
{
protected:
	IKGizmoPtr m_Gizmo;
	KCamera* m_Camera;
	IKRenderWindow* m_Window;

	KKeyboardCallbackType m_KeyCallback;
	KMouseCallbackType m_MouseCallback;

	bool m_Enable;
public:
	KGizmoController();
	~KGizmoController();

	void SetEnable(bool enable) { m_Enable = enable; }

	bool Init(IKGizmoPtr gizmo, KCamera* camera, IKRenderWindow* window);
	bool UnInit();

	bool Update(float dt);
};