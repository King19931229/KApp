#pragma once
#include "KGizmoBase.h"
#include "Internal/ECS/KECSGlobal.h"

class KScaleGizmo : public KGizmoBase
{
protected:
	KPlane m_PickPlane;
	glm::vec3 m_IntersectPos;
	glm::vec2 m_PickPos;

	IKEntityPtr m_OriginEntity;

	IKEntityPtr m_XAxisEntity;
	IKEntityPtr m_YAxisEntity;
	IKEntityPtr m_ZAxisEntity;

	IKEntityPtr m_XCubeEntity;
	IKEntityPtr m_YCubeEntity;
	IKEntityPtr m_ZCubeEntity;

	IKEntityPtr m_XZPlaneEntity;
	IKEntityPtr m_YZPlaneEntity;
	IKEntityPtr m_XYPlaneEntity;

	IKEntityPtr m_XYZPlaneEntity;

	enum class ScaleOperator
	{
		SCALE_X,
		SCALE_Y,
		SCALE_Z,
		SCALE_XY,
		SCALE_XZ,
		SCALE_YZ,
		SCALE_XYZ,
		SCALE_NONE
	};
	ScaleOperator m_OperatorType;

	ScaleOperator GetOperatorType(unsigned int x, unsigned int y, KPlane& plane, glm::vec3& intersectPos);

	glm::vec3 GetScale() override;
public:
	KScaleGizmo();
	~KScaleGizmo();

	GizmoType GetType() const override { return GizmoType::GIZMO_TYPE_SCALE; }

	bool Init(const KCamera* camera) override;
	bool UnInit() override;

	void OnMouseDown(unsigned int x, unsigned int y) override;
	void OnMouseMove(unsigned int x, unsigned int y) override;
	void OnMouseUp(unsigned int x, unsigned int y) override;

	bool IsTriggered() const final;
};