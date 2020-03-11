#pragma once
#include "KGizmoBase.h"
#include "Internal/ECS/KECSGlobal.h"

class KRotateGizmo : public KGizmoBase
{
protected:
	KPlane m_PickPlane;
	glm::vec3 m_IntersectPos;
	glm::mat4 m_PickTransform;

	KEntityPtr m_XPlaneEntity;
	KEntityPtr m_YPlaneEntity;
	KEntityPtr m_ZPlaneEntity;

	KEntityPtr m_XRotateEntity;
	KEntityPtr m_YRotateEntity;
	KEntityPtr m_ZRotateEntity;

	enum class RotateOperator
	{
		ROTATE_X,
		ROTATE_Y,
		ROTATE_Z,
		ROTATE_SCREEN,
		ROTATE_TWIN,
		ROTATE_NONE
	};
	RotateOperator m_OperatorType;

	RotateOperator GetOperatorType(unsigned int x, unsigned int y, KPlane& plane, glm::vec3& intersectPos);
public:
	KRotateGizmo();
	~KRotateGizmo();

	GizmoType GetType() const override { return GizmoType::GIZMO_TYPE_ROTATE; }

	bool Init(const KCamera* camera) override;
	bool UnInit() override;

	void OnMouseDown(unsigned int x, unsigned int y) override;
	void OnMouseMove(unsigned int x, unsigned int y) override;
	void OnMouseUp(unsigned int x, unsigned int y) override;
};