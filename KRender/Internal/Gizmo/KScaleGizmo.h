#pragma once
#include "KGizmoBase.h"
#include "Internal/ECS/KECSGlobal.h"

class KScaleGizmo : public KGizmoBase
{
protected:
	KPlane m_PickPlane;
	glm::vec3 m_IntersectPos;
	glm::vec2 m_PickPos;

	KEntityPtr m_OriginEntity;

	KEntityPtr m_XAxisEntity;
	KEntityPtr m_YAxisEntity;
	KEntityPtr m_ZAxisEntity;

	KEntityPtr m_XCubeEntity;
	KEntityPtr m_YCubeEntity;
	KEntityPtr m_ZCubeEntity;

	KEntityPtr m_XZPlaneEntity;
	KEntityPtr m_YZPlaneEntity;
	KEntityPtr m_XYPlaneEntity;

	KEntityPtr m_XYZPlaneEntity;

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
};