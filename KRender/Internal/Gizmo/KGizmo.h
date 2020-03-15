#pragma once
#include "Interface/IKGizmo.h"
#include "KMoveGizmo.h"
#include "KRotateGizmo.h"
#include "KScaleGizmo.h"

class KGizmo : public IKGizmoGroup
{
protected:
	IKGizmoPtr m_TranslateGizmo;
	IKGizmoPtr m_RotateGizmo;
	IKGizmoPtr m_ScaleGizmo;

	IKGizmoPtr m_CurrentGizmo;
public:
	KGizmo();
	~KGizmo();

	GizmoType GetType() const override;

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

	GizmoType SetType(GizmoType type) override;
};