#pragma once
#include "Interface/IKGizmo.h"
#include "KBase/Interface/Entity/IKEntityManager.h"

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

	void SetEntityColor(IKEntityPtr entity, const glm::vec4& color);
	bool CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir);

	enum class GizmoAxis
	{
		AXIS_X,
		AXIS_Y,
		AXIS_Z
	};
	glm::vec3 GetAxis(GizmoAxis axis);

	std::vector<IKEntityPtr> m_AllEntity;
	std::vector<KGizmoTransformCallback*> m_TransformCallback;
	std::vector<KGizmoTriggerCallback*> m_TriggerCallback;

	virtual glm::mat3 GetRotate(IKEntityPtr entity, const glm::mat3& gizmoRotate);
	virtual glm::vec3 GetScale();

	glm::vec3 TransformPos() const;
	glm::vec3 TransformScale() const;
	glm::mat3 TransformRotate() const;

	template<typename T, typename Container>
	bool RegisterCallback(Container& container, T* callback)
	{
		if (callback && std::find(container.begin(), container.end(), callback) == container.end())
		{
			container.push_back(callback);
			return true;
		}
		return false;
	}

	template<typename T, typename Container>
	bool UnRegisterCallback(Container& container, T* callback)
	{
		if (callback)
		{
			auto it = std::find(container.begin(), container.end(), callback);
			if (it != container.end())
			{
				container.erase(it);
				return true;
			}
		}
		return false;
	}

	void OnTriggerCallback(bool trigger);
	void OnTransformCallback(const glm::mat4& transform);
public:
	KGizmoBase();
	~KGizmoBase();

	bool Init(const KCamera* camera) override;
	bool UnInit() override;

	bool SetType(GizmoType type) final { return false; }

	void Enter() final;
	void Leave() final;

	void Update() final;

	const glm::mat4& GetMatrix() const final;
	void SetMatrix(const glm::mat4& matrix) final;

	GizmoManipulateMode GetManipulateMode() const final;
	void SetManipulateMode(GizmoManipulateMode mode) final;

	float GetDisplayScale() const final;
	void SetDisplayScale(float scale) final;

	void SetScreenSize(unsigned int width, unsigned int height) final;

	bool RegisterTransformCallback(KGizmoTransformCallback* callback) final;
	bool UnRegisterTransformCallback(KGizmoTransformCallback* callback) final;

	bool RegisterTriggerCallback(KGizmoTriggerCallback* callback) final;
	bool UnRegisterTriggerCallback(KGizmoTriggerCallback* callback) final;
};