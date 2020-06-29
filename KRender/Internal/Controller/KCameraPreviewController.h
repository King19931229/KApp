#pragma once
#include "Interface/IKRenderWindow.h"
#include "Interface/IKGizmo.h"
#include "Interface/IKCameraController.h"
#include "Publish/KCamera.h"

class KCameraPreviewController : public IKCameraPreviewController
{
protected:
	KCamera* m_Camera;
	IKRenderWindow* m_Window;
	KMouseCallbackType m_MouseCallback;
	KScrollCallbackType m_ScrollCallback;
	glm::vec3 m_PreviewCenter;
	bool m_MouseDown[INPUT_MOUSE_BUTTON_COUNT];
	float m_MousePos[2];
	float m_PreviewDistance;
	bool m_Enable;

	void ZeroData();
public:
	KCameraPreviewController();
	~KCameraPreviewController();

	bool Init(KCamera* camera, IKRenderWindow* window) override;
	bool UnInit() override;
	void SetEnable(bool enable) override;
	void SetPreviewCenter(const glm::vec3& center) override;
	void Update() override;
};