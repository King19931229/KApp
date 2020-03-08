#pragma once
#include "Interface/IKGizmo.h"
#include "Internal/ECS/KECSGlobal.h"

class KMoveGizmo : public IKGizmo
{
protected:
	glm::mat4 m_Transform;
	GizmoManipulateMode m_Mode;
	unsigned int m_ScreenWidth;
	unsigned int m_ScreenHeight;
	float m_DisplayScale;
	float m_ScreenScaleFactor;
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

	bool CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir);

	enum class MoveGizmoAxis
	{
		AXIS_X,
		AXIS_Y,
		AXIS_Z
	};
	glm::vec3 GetAxis(MoveGizmoAxis axis);

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

	MoveOperator GetOperatorType(unsigned int x, unsigned int y);
public:
	KMoveGizmo();
	~KMoveGizmo();

	GizmoType GetType() const override { return GizmoType::GIZMO_TYPE_MOVE; }

	bool Init(const KCamera* camera) override;
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
};