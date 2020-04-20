#pragma once
#include "KGizmoBase.h"
#include "Internal/ECS/KECSGlobal.h"

class KMoveGizmo : public KGizmoBase
{
protected:
	KPlane m_PickPlane;
	glm::vec3 m_IntersectPos;

	IKEntityPtr m_OriginEntity;

	IKEntityPtr m_XAxisEntity;
	IKEntityPtr m_YAxisEntity;
	IKEntityPtr m_ZAxisEntity;

	IKEntityPtr m_XArrowEntity;
	IKEntityPtr m_YArrowEntity;
	IKEntityPtr m_ZArrowEntity;

	IKEntityPtr m_XZPlaneEntity;
	IKEntityPtr m_YZPlaneEntity;
	IKEntityPtr m_XYPlaneEntity;

	enum class MoveOperator
	{
		MOVE_X,
		MOVE_Y,
		MOVE_Z,
		MOVE_XY,
		MOVE_XZ,
		MOVE_YZ,
		MOVE_NONE
	};
	MoveOperator m_OperatorType;

	MoveOperator GetOperatorType(unsigned int x, unsigned int y, KPlane& plane, glm::vec3& intersectPos);
public:
	KMoveGizmo();
	~KMoveGizmo();

	GizmoType GetType() const override { return GizmoType::GIZMO_TYPE_MOVE; }

	bool Init(const KCamera* camera) override;
	bool UnInit() override;

	void OnMouseDown(unsigned int x, unsigned int y) override;
	void OnMouseMove(unsigned int x, unsigned int y) override;
	void OnMouseUp(unsigned int x, unsigned int y) override;

	bool IsTriggered() const final;
};