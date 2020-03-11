#pragma once
#include "Interface/IKGizmo.h"
#include "Internal/ECS/KECSGlobal.h"

class KGizmoBase : public IKGizmo
{
protected:
	glm::mat4 m_Transform;
	GizmoManipulateMode m_Mode;
	unsigned int m_ScreenWidth;
	unsigned int m_ScreenHeight;
	float m_DisplayScale;
	float m_ScreenScaleFactor;
	const KCamera* m_Camera;

	bool CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir);

	enum class GizmoAxis
	{
		AXIS_X,
		AXIS_Y,
		AXIS_Z
	};
	glm::vec3 GetAxis(GizmoAxis axis);

	std::vector<KEntityPtr> m_AllEntity;
public:
	KGizmoBase();
	~KGizmoBase();

	bool Init(const KCamera* camera) override;
	bool UnInit() override;

	void Enter() final;
	void Leave() final;

	void Update() override;

	const glm::mat4& GetMatrix() const final;
	void SetMatrix(const glm::mat4& matrix) final;

	GizmoManipulateMode GetManipulateMode() const final;
	void SetManipulateMode(GizmoManipulateMode mode) final;

	float GetDisplayScale() const final;
	void SetDisplayScale(float scale) final;

	void SetScreenSize(unsigned int width, unsigned int height) final;
};