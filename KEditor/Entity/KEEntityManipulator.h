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

class KESceneItemWidget;
class KEEntityManipulator
{
	friend class KEEntitySceneJoinCommand;
	friend class KEEntitySceneEraseCommand;
	friend class KEEntitySceneTransformCommand;
	friend class KEEntitySelector;
public:
	typedef std::unordered_map<IKEntity::IDType, KEEntityPtr> EntityCollectionType;
protected:	
	IKGizmoPtr m_Gizmo;
	IKRenderWindow* m_Window;
	const KCamera* m_Camera;
	IKScene* m_Scene;
	KESceneItemWidget* m_SceneItemWidget;

	SelectType m_SelectType;

	glm::mat4 m_PreviousTransform;

	KKeyboardCallbackType m_KeyboardCallback;
	KMouseCallbackType m_MouseCallback;
	KGizmoTransformCallback m_TransformCallback;

	KGizmoTriggerCallback m_TriggerCallback;

	EntityCollectionType m_Entities;

	float m_MouseDownPos[2];
	bool m_Enable;

	void OnKeyboardListen(InputKeyboard key, InputAction action);
	void OnMouseListen(InputMouseButton key, InputAction action, float x, float y);
	void OnGizmoTransformChange(const glm::mat4& transform);
	void OnGizmoTrigger(bool trigger);

	KEEntityPtr GetEditorEntity(IKEntityPtr entity);
	void OnSelectionDelete();

	void AddEditorEntity(KEEntityPtr editorEntity);
	void RemoveEditorEntity(IKEntity::IDType id);
	
	void WatchEntity(KEEntityPtr editorEntity);
	void DiscardEntity(KEEntityPtr editorEntity);
public:
	KEEntityManipulator();
	~KEEntityManipulator();

	bool Init(IKGizmoPtr gizmo, IKRenderWindow* window, const KCamera* camera, IKScene* scene, KESceneItemWidget* sceneItemWidget);
	bool UnInit();

	bool Join(IKEntityPtr entity, const std::string& path);
	bool Erase(KEEntityPtr editorEntity);
	bool Erase(const std::vector<KEEntityPtr>& entites);

	bool Load(const char* filename);
	bool Save(const char* filename);

	SelectType GetSelectType() const;
	bool SetSelectType(SelectType type);

	GizmoType GetGizmoType() const;
	bool SetGizmoType(GizmoType type);

	GizmoManipulateMode GetManipulateMode() const;
	bool SetManipulateMode(GizmoManipulateMode mode);

	KEEntityPtr GetEntity(IKEntity::IDType id);
	KEEntityPtr CloestPickEntity(size_t x, size_t y);

	void UpdateGizmoTransform();
};