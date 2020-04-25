#pragma once

#include "KBase/Interface/Entity/IKEntity.h"
#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKGizmo.h"
#include "KRender/Publish/KCamera.h"
#include "KRender/Interface/IKRenderScene.h"
#include "KEditorConfig.h"
#include <unordered_map>

enum class SelectType
{
	SELECT_TYPE_SINGLE,
	SELECT_TYPE_MULTI
};

class KEEntityManipulator
{
	friend class KEEntitySceneJoinCommand;
	friend class KEEntitySceneEraseCommand;
	friend class KEEntitySceneTransformCommand;
public:
	typedef std::unordered_map<IKEntity::IDType, KEEntityPtr> EntityCollectionType;
protected:	
	IKGizmoPtr m_Gizmo;
	IKRenderWindow* m_Window;
	const KCamera* m_Camera;
	// TODO IKScene
	IKRenderScene* m_Scene;

	SelectType m_SelectType;

	glm::mat4 m_PreviousTransform;

	KKeyboardCallbackType m_KeyboardCallback;
	KMouseCallbackType m_MouseCallback;
	KGizmoTransformCallback m_TransformCallback;

	KGizmoTriggerCallback m_TriggerCallback;

	EntityCollectionType m_Entities;
	EntityCollectionType m_SelectEntites;

	float m_MouseDownPos[2];
	bool m_Enable;

	void OnKeyboardListen(InputKeyboard key, InputAction action);
	void OnMouseListen(InputMouseButton key, InputAction action, float x, float y);
	void OnGizmoTransformChange(const glm::mat4& transform);
	void UpdateGizmoTransform();
	void OnGizmoTrigger(bool trigger);

	KEEntityPtr GetEditorEntity(IKEntityPtr entity);
	void OnSelectionDelete();

	void AddEditorEntity(KEEntityPtr editorEntity);
	void RemoveEditorEntity(IKEntity::IDType id);
public:
	KEEntityManipulator();
	~KEEntityManipulator();

	bool Init(IKGizmoPtr gizmo, IKRenderWindow* window, const KCamera* camera, IKRenderScene* scene);
	bool UnInit();

	bool Join(IKEntityPtr entity, const std::string& path);
	bool Erase(KEEntityPtr editorEntity);

	SelectType GetSelectType() const;
	bool SetSelectType(SelectType type);

	GizmoType GetGizmoType() const;
	bool SetGizmoType(GizmoType type);

	GizmoManipulateMode GetManipulateMode() const;
	bool SetManipulateMode(GizmoManipulateMode mode);
};