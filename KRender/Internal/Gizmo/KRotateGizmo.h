#pragma once
#include "KGizmoBase.h"
#include "Internal/ECS/KECSGlobal.h"

class KRotateGizmo : public KGizmoBase
{
protected:
	KPlane m_PickPlane;
	glm::vec3 m_IntersectPos;
	glm::mat4 m_PickTransform;

	IKEntityPtr m_XPlaneEntity;
	IKEntityPtr m_YPlaneEntity;
	IKEntityPtr m_ZPlaneEntity;

	IKEntityPtr m_RotateEntity;

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

	glm::mat3 GetRotate(IKEntityPtr entity, const glm::mat3& gizmoRotate) override;
public:
	KRotateGizmo();
	~KRotateGizmo();

	GizmoType GetType() const override { return GizmoType::GIZMO_TYPE_ROTATE; }

	bool Init(const KCamera* camera) override;
	bool UnInit() override;
	void OnMouseDown(unsigned int x, unsigned int y) override;
	void OnMouseMove(unsigned int x, unsigned int y) override;
	void OnMouseUp(unsigned int x, unsigned int y) override;

	bool IsTriggered() const final;
};