#pragma once
#include "KRender/Interface/IKRenderScene.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Publish/KMath.h"
#include "Internal/KConstantDefinition.h"

class KTransformComponent : public IKTransformComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKTransformComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
	IKRenderScene* m_RenderScene;
	glm::vec3 m_Position;
	glm::vec3 m_Scale;
	glm::quat m_Rotate;	
	bool m_IsStatic;
	
	KConstantDefinition::OBJECT m_FinalTransform;
	KConstantDefinition::OBJECT m_FinalTransform_RenderThread;

	std::vector<KTransformChangeCallback*> m_TransformChangeCallbacks;

	static constexpr const char* msPosition = "position";
	static constexpr const char* msScale = "scale";
	static constexpr const char* msRotate = "rotate";
	static constexpr const char* msIsStatic = "static";

	void UpdateTransform();
public:
	KTransformComponent();
	virtual ~KTransformComponent();

	bool Save(IKXMLElementPtr element) override;
	bool Load(IKXMLElementPtr element) override;

	bool Tick(float dt) override;

	bool RegisterTransformChangeCallback(KTransformChangeCallback* callback) override;
	bool UnRegisterTransformChangeCallback(KTransformChangeCallback* callback) override;

	const glm::quat& GetRotateQuat() const override;
	const glm::vec3& GetScale() const override;
	const glm::vec3& GetPosition() const override;
	glm::vec3 GetRotateEularAngles() const override;
	bool IsStatic() const override;
	void SetRotateQuat(const glm::quat& rotate) override;
	void SetRotateMatrix(const glm::mat3& rotate) override;
	void SetRotateEularAngles(glm::vec3 eularAngles) override;
	void SetScale(const glm::vec3& scale) override;
	void SetPosition(const glm::vec3& position) override;
	const glm::mat4& GetFinal_GameThread() const override;
	const glm::mat4& GetPrevFinal_GameThread() const override;
	const glm::mat4& GetFinal_RenderThread() const override;
	const glm::mat4& GetPrevFinal_RenderThread() const override;
	const glm::mat4& GetFinal() const override;
	const glm::mat4& GetPrevFinal() const override;
	void SetFinal(const glm::mat4& transform) override;
	void SetStatic(bool isStatic) override;

	void SetRenderScene(IKRenderScene* renderScene);
	const KConstantDefinition::OBJECT& FinalTransform_RenderThread() const;
};