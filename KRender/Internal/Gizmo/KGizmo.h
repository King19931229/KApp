#pragma once
#include "Interface/IKGizmo.h"
#include "KMoveGizmo.h"
#include "KRotateGizmo.h"
#include "KScaleGizmo.h"

class KGizmo : public IKGizmo
{
protected:
	IKGizmoPtr m_TranslateGizmo;
	IKGizmoPtr m_RotateGizmo;
	IKGizmoPtr m_ScaleGizmo;
	IKGizmoPtr m_CurrentGizmo;
	bool m_Enter;
public:
	KGizmo();
	~KGizmo();

	GizmoType GetType() const override;
	bool SetType(GizmoType type) override;

	bool Init(const KCamera* camera)  override;
	bool UnInit() override;

	void Enter() override;
	void Leave() override;

	void Update() override;

	const glm::mat4& GetMatrix() const override;
	void SetMatrix(const glm::mat4& matrix) override;

	GizmoManipulateMode GetManipulateMode() const override;
	void SetManipulateMode(GizmoManipulateMode mode) override;

	float GetDisplayScale() const override;
	void SetDisplayScale(float scale) override;

	void SetScreenSize(unsigned int width, unsigned int height) override;

	void OnMouseDown(unsigned int x, unsigned int y) override;
	void OnMouseMove(unsigned int x, unsigned int y) override;
	void OnMouseUp(unsigned int x, unsigned int y) override;

	bool RegisterTransformCallback(KGizmoTransformCallback* callback) override;
	bool UnRegisterTransformCallback(KGizmoTransformCallback* callback) override;

	bool RegisterTriggerCallback(KGizmoTriggerCallback* callback) override;
	bool UnRegisterTriggerCallback(KGizmoTriggerCallback* callback) override;

	bool IsTriggered() const override;
};