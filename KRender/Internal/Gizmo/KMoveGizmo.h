#pragma once
#include "Interface/IKGizmo.h"
#include "Internal/ECS/KECSGlobal.h"

class KMoveGizmo : public IKGizmo
{
protected:
	glm::mat4 m_Transform;
	GizmoManipulateMode m_Mode;
	const KCamera* m_Camera;

	KEntityPtr m_OriginEntity;

	KEntityPtr m_XAxisEntity;
	KEntityPtr m_YAxisEntity;
	KEntityPtr m_ZAxisEntity;

	KEntityPtr m_XArrowEntity;
	KEntityPtr m_YArrowEntity;
	KEntityPtr m_ZArrowEntity;

	KEntityPtr m_XZPlaneEntity;
	KEntityPtr m_YZPlaneEntity;
	KEntityPtr m_XYPlaneEntity;
public:
	KMoveGizmo();
	~KMoveGizmo();

	GizmoType GetType() const override { return GizmoType::GIZMO_TYPE_MOVE; }

	bool Init(const KCamera* camera) override;
	bool UnInit() override;

	void Enter() override;
	void Leave() override;

	const glm::mat4& GetMatrix() const override;

	GizmoManipulateMode GetManipulateMode() const override;
	void SetManipulateMode(GizmoManipulateMode mode) override;
};