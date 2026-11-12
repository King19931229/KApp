#include "KEEntityManipulator.h"
#include "KEEntityCommand.h"
#include "KEditorGlobal.h"

#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Publish/KMath.h"

#include <assert.h>

KEEntityManipulator::KEEntityManipulator()
	: m_Gizmo(nullptr),
	m_Window(nullptr),
	m_Camera(nullptr),
	m_Scene(nullptr),
	m_SelectType(SelectType::SELECT_TYPE_SINGLE),
	m_Enable(false),
	m_PreviousTransform(glm::mat4(1.0f)),
	m_GizmoMouseDown(false)
{
	m_MouseDownPos[0] = 0.0f;
	m_MouseDownPos[1] = 0.0f;
}

KEEntityManipulator::~KEEntityManipulator()
{
	assert(!m_Gizmo);
}

bool KEEntityManipulator::Init(IKGizmoPtr gizmo, IKRenderWindow* window, const KCamera* camera, IKScene* scene)
{
	if (gizmo && window && camera && scene)
	{
		m_Gizmo = gizmo;
		m_Window = window;
		m_Camera = camera;
		m_Scene = scene;

		m_KeyboardCallback = [this](InputKeyboard key, InputAction action)
		{
			OnKeyboardListen(key, action);
		};
		m_MouseCallback = [this](InputMouseButton key, InputAction action, float x, float y)
		{
			OnMouseListen(key, action, x, y);
		};
		m_TransformCallback = [this](const glm::mat4& transform)
		{
			OnGizmoTransformChange(transform);
		};
		m_TriggerCallback = [this](bool trigger)
		{
			OnGizmoTrigger(trigger);
		};

		m_Window->RegisterKeyboardCallback(&m_KeyboardCallback);
		m_Window->RegisterMouseCallback(&m_MouseCallback);
		m_Gizmo->RegisterTransformCallback(&m_TransformCallback);
		m_Gizmo->RegisterTriggerCallback(&m_TriggerCallback);

		return true;
	}
	return false;
}

bool KEEntityManipulator::UnInit()
{
	if (m_Gizmo)
	{
		m_Gizmo->UnRegisterTransformCallback(&m_TransformCallback);
		m_Gizmo->UnRegisterTriggerCallback(&m_TriggerCallback);
		m_Gizmo = nullptr;
	}

	if (m_Window)
	{
		m_Window->UnRegisterKeyboardCallback(&m_KeyboardCallback);
		m_Window->UnRegisterMouseCallback(&m_MouseCallback);
		m_Window = nullptr;
	}

	KEditorGlobal::EntitySelector.Clear();
	m_Entities.clear();
	m_GizmoMouseDown = false;

	m_Camera = nullptr;
	m_Scene = nullptr;

	return true;
}

void KEEntityManipulator::AddEditorEntity(KEEntityPtr editorEntity)
{
	if (editorEntity)
	{
		m_Scene->Add(editorEntity->soul);
		WatchEntity(editorEntity);
	}
}

void KEEntityManipulator::WatchEntity(KEEntityPtr editorEntity)
{
	if (editorEntity)
	{
		KReflectionObjectBase* reflection = nullptr;
		editorEntity->soul->QueryReflection(&reflection);
		ASSERT_RESULT(reflection);

		m_Entities[editorEntity->soul->GetID()] = editorEntity;
	}
}

void KEEntityManipulator::RemoveEditorEntity(IKEntity::IDType id)
{
	KEditorGlobal::EntitySelector.Remove(id);
	auto it = m_Entities.find(id);
	if (it != m_Entities.end())
	{
		KEEntityPtr editorEntity = it->second;
		editorEntity->createInfo.name = editorEntity->soul->GetName();
		editorEntity->soul->GetTransform(editorEntity->createInfo.transform);
		IKTransformComponent* transform = nullptr;
		if (editorEntity->soul->GetComponent(CT_TRANSFORM, &transform)) editorEntity->createInfo.isStatic = transform->IsStatic();
		m_Scene->Remove(editorEntity->soul);
		KEditorGlobal::ResourcePorter.UnInitEntity(editorEntity->soul, editorEntity->createInfo.allocatedName);
		editorEntity->createInfo.allocatedName.clear();
		m_Entities.erase(it);
	}
}

bool KEEntityManipulator::Join(IKEntityPtr entity, const std::string& path)
{
	if (entity)
	{
		KEEntityPtr editorEntity = KEEntityPtr(KNEW KEEntity());
		editorEntity->soul = entity;

		editorEntity->createInfo.path = path;
		editorEntity->createInfo.name = entity->GetName();
		editorEntity->createInfo.allocatedName = entity->GetName();
		IKTransformComponent* transformComponent = nullptr;
		if (entity->GetComponent(CT_TRANSFORM, &transformComponent)) editorEntity->createInfo.isStatic = transformComponent->IsStatic();

		glm::mat4 transform = glm::mat4(1.0f);
		ASSERT_RESULT(entity->GetTransform(transform));
		editorEntity->createInfo.transform = transform;

		AddEditorEntity(editorEntity);

		KECommandPtr command = KECommandPtr(KNEW KEEntitySceneJoinCommand(editorEntity, m_Scene, this));
		KEditorGlobal::CommandInvoker.Push(command);

		return true;
	}
	return false;
}

bool KEEntityManipulator::Erase(KEEntityPtr editorEntity)
{
	if (editorEntity)
	{
		RemoveEditorEntity(editorEntity->soul->GetID());
		KECommandPtr command = KECommandPtr(KNEW KEEntitySceneEraseCommand(editorEntity,
			m_Scene, this));
		KEditorGlobal::CommandInvoker.Push(command);
		return true;
	}
	return false;
}

bool KEEntityManipulator::Erase(const std::vector<KEEntityPtr>& entites)
{
	if (!entites.empty())
	{
		KECommandPtr command = KECommandPtr(KNEW KEEntitySceneEraseCommand(m_Scene, this));
		for (KEEntityPtr entity : entites)
		{
			if (entity)
			{
				RemoveEditorEntity(entity->soul->GetID());
				command->Cast<KEEntitySceneEraseCommand>()->Append(entity);
			}
		}
		KEditorGlobal::CommandInvoker.Push(command);
		return true;
	}
	return false;
}

bool KEEntityManipulator::Load(const char* filename)
{
	KEditorGlobal::EntitySelector.Clear();
	m_Scene->Clear();
	for (const auto& entry : m_Entities) KEditorGlobal::EntityNamePool.FreeName(entry.second->createInfo.allocatedName);
	m_Entities.clear();

	KEditorGlobal::CommandInvoker.Clear();

	KECommandPtr command = KECommandPtr(KNEW KEEntitySceneJoinCommand(m_Scene, this));

	if (m_Scene->Load(filename))
	{
		auto entities = m_Scene->GetEntities();
		for (IKEntityPtr entity : entities)
		{
			IKRenderComponent* renderComponent = nullptr;
			if (entity->GetComponent(CT_RENDER, &renderComponent))
			{
				std::string path;
				if (renderComponent->GetPath(path))
				{
					KEEntityPtr editorEntity = KEEntityPtr(KNEW KEEntity());
					editorEntity->soul = entity;

					editorEntity->createInfo.path = path;
					editorEntity->createInfo.name = entity->GetName();
					IKTransformComponent* transformComponent = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, &transformComponent)) editorEntity->createInfo.isStatic = transformComponent->IsStatic();

					glm::mat4 transform = glm::mat4(1.0f);
					ASSERT_RESULT(entity->GetTransform(transform));
					editorEntity->createInfo.transform = transform;

					WatchEntity(editorEntity);

					command->Cast<KEEntitySceneJoinCommand>()->Append(editorEntity);
				}
			}
		}
		KEditorGlobal::CommandInvoker.Push(command);
		return true;
	}
	return false;
}

bool KEEntityManipulator::Save(const char* filename)
{
	return m_Scene->Save(filename);
}

KEEntityPtr KEEntityManipulator::GetEditorEntity(IKEntity* entity)
{
	if (entity)
	{
		auto it = m_Entities.find(entity->GetID());
		if (it != m_Entities.end())
		{
			return it->second;
		}
	}
	return nullptr;
}

void KEEntityManipulator::OnKeyboardListen(InputKeyboard key, InputAction action)
{
	if (key == INPUT_KEY_CTRL)
	{
		if (action == INPUT_ACTION_PRESS)
		{
			m_SelectType = SelectType::SELECT_TYPE_MULTI;
		}
		else
		{
			m_SelectType = SelectType::SELECT_TYPE_SINGLE;
		}
	}
}

void KEEntityManipulator::OnMouseListen(InputMouseButton key, InputAction action, float x, float y)
{
	if (key == INPUT_MOUSE_BUTTON_LEFT)
	{
		if (action == INPUT_ACTION_PRESS)
		{
			m_MouseDownPos[0] = x;
			m_MouseDownPos[1] = y;
		}
		else if (action == INPUT_ACTION_RELEASE)
		{
			if (m_GizmoMouseDown)
			{
				m_GizmoMouseDown = false;
				return;
			}
			// 点击操作
			float dx = x - m_MouseDownPos[0];
			float dy = y - m_MouseDownPos[1];
			if (dx * dx + dy * dy <= 9.0f)
			{
				IKEntity* entity = nullptr;
				KEEntityPtr editorEntity;

				size_t width = 0;
				size_t height = 0;
				m_Window->GetInputSize(width, height);

				if (m_SelectType == SelectType::SELECT_TYPE_SINGLE)
				{
					KEditorGlobal::EntitySelector.Clear();
				}

				if (m_Scene->CloestPick(*m_Camera, (size_t)x, (size_t)y,
					width, height, entity) && (editorEntity = GetEditorEntity(entity)))
				{
					if (m_SelectType == SelectType::SELECT_TYPE_SINGLE)
					{
						KEditorGlobal::EntitySelector.Add(editorEntity);
					}
					else if (m_SelectType == SelectType::SELECT_TYPE_MULTI)
					{
						if (!KEditorGlobal::EntitySelector.Contain(entity->GetID()))
						{
							KEditorGlobal::EntitySelector.Add(editorEntity);
						}
						else
						{
							KEditorGlobal::EntitySelector.Remove(editorEntity);
						}
					}
					UpdateGizmoTransform();
				}
			}
			// 拉框多选
			else
			{

			}
		}
	}

}

void KEEntityManipulator::OnGizmoTransformChange(const glm::mat4& transform)
{
	if (!KEditorGlobal::EntitySelector.Empty())
	{
		glm::vec3 deltaTranslate = KMath::ExtractPosition(transform) - KMath::ExtractPosition(m_PreviousTransform);
		glm::mat3 deltaRotate = KMath::ExtractRotate(transform) * glm::inverse(KMath::ExtractRotate(m_PreviousTransform));
		glm::vec3 deltaScale = KMath::ExtractScale(transform) / KMath::ExtractScale(m_PreviousTransform);

		/*
		// 以下方法无效 由于整体变换 = T * R * S
		// 不能直接用逆矩阵的提取
		glm::mat4 deltaTransform = transform * glm::inverse(m_PreviousTransform);
		glm::vec3 deltaTranslate = KMath::ExtractPosition(deltaTransform);
		glm::mat3 deltaRotate = KMath::ExtractRotate(deltaTransform);
		glm::vec3 deltaScale = KMath::ExtractScale(deltaTransform);
		*/

		GizmoManipulateMode mode = GetManipulateMode();
		GizmoType type = GetGizmoType();

		const auto& selections = KEditorGlobal::EntitySelector.GetSelection();
		for (auto& pair : selections)
		{
			KEEntityPtr editorEntity = pair.second;

			IKTransformComponent* transformComponent = nullptr;
			if (editorEntity->soul->GetComponent(CT_TRANSFORM, &transformComponent))
			{
				if (type == GizmoType::GIZMO_TYPE_MOVE)
				{
					glm::vec3 pos = deltaTranslate + transformComponent->GetPosition();
					transformComponent->SetPosition(pos);
				}
				else if (type == GizmoType::GIZMO_TYPE_SCALE)
				{
					glm::vec3 scale = deltaScale * transformComponent->GetScale();
					transformComponent->SetScale(scale);
				}
				else if (type == GizmoType::GIZMO_TYPE_ROTATE)
				{
					if (mode == GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL)
					{
						glm::mat3 rotate = deltaRotate * glm::mat3_cast(transformComponent->GetRotateQuat());
						transformComponent->SetRotateQuat(rotate);
					}
					else
					{
						glm::vec3 gizmoPos = KMath::ExtractPosition(transform);
						glm::vec3 releatedPos = transformComponent->GetPosition() - gizmoPos;
						releatedPos = glm::mat4(deltaRotate) * glm::vec4(releatedPos, 1.0f);
						transformComponent->SetPosition(glm::vec3(releatedPos) + gizmoPos);

						glm::mat3 rotate = deltaRotate * glm::mat3_cast(transformComponent->GetRotateQuat());
						transformComponent->SetRotateQuat(rotate);
					}
				}
				// 覆盖初始位置 保证Undo Redo正确
				editorEntity->soul->GetTransform(editorEntity->createInfo.transform);
			}
		}
	}

	m_PreviousTransform = transform;
}

void KEEntityManipulator::UpdateGizmoTransform()
{
	if (!m_Gizmo) return;
	const auto& selections = KEditorGlobal::EntitySelector.GetSelection();

	if (selections.empty())
	{
		m_Gizmo->Leave();
		return;
	}
	m_Gizmo->Enter();

	if (selections.size() > 1)
	{
		glm::vec3 pos = glm::vec3(0.0f);
		int num = 0;
		for (auto& pair : selections)
		{
			KEEntityPtr entity = pair.second;

			IKTransformComponent* transformComponent = nullptr;
			if (entity->soul->GetComponent(CT_TRANSFORM, &transformComponent))
			{
				pos += transformComponent->GetPosition();
				++num;
			}
		}
		if (num)
		{
			pos = pos / (float)num;
			m_Gizmo->SetMatrix(glm::translate(glm::mat4(1.0f), pos));
		}
	}
	else
	{
		KEEntityPtr entity = (*selections.begin()).second;
		IKTransformComponent* transformComponent = nullptr;
		if (entity->soul->GetComponent(CT_TRANSFORM, &transformComponent))
		{
			m_Gizmo->SetMatrix(transformComponent->GetFinal_GameThread());
		}
	}

	m_PreviousTransform = m_Gizmo->GetMatrix();
}

void KEEntityManipulator::OnGizmoTrigger(bool trigger)
{
	const auto& selections = KEditorGlobal::EntitySelector.GetSelection();

	if (trigger)
	{
		m_GizmoMouseDown = true;
		for (auto& pair : selections)
		{
			KEEntityPtr editorEntity = pair.second;
			editorEntity->soul->GetTransform(editorEntity->transformInfo.previous);
		}
	}
	else if(!selections.empty())
	{
		KECommandPtr command = KECommandPtr(KNEW KEEntitySceneTransformCommand(m_Scene, this));
		bool changed = false;
		for (auto& pair : selections)
		{
			KEEntityPtr editorEntity = pair.second;
			editorEntity->soul->GetTransform(editorEntity->transformInfo.current);

			if (editorEntity->transformInfo.previous != editorEntity->transformInfo.current)
			{
				command->Cast<KEEntitySceneTransformCommand>()->Append(editorEntity, editorEntity->transformInfo.previous, editorEntity->transformInfo.current);
				changed = true;
			}
		}
		if (changed) KEditorGlobal::CommandInvoker.Push(command);
	}
}

SelectType KEEntityManipulator::GetSelectType() const
{
	return m_SelectType;
}

bool KEEntityManipulator::SetSelectType(SelectType type)
{
	m_SelectType = type;
	return true;
}

GizmoType KEEntityManipulator::GetGizmoType() const
{
	if (m_Gizmo)
	{
		GizmoType type = m_Gizmo->GetType();
		return type;
	}
	assert(false && "gizmo not set");
	return GizmoType::GIZMO_TYPE_MOVE;
}

bool KEEntityManipulator::SetGizmoType(GizmoType type)
{
	if (m_Gizmo)
	{
		m_Gizmo->SetType(type);
		return true;
	}
	return false;
}

GizmoManipulateMode KEEntityManipulator::GetManipulateMode() const
{
	if (m_Gizmo)
	{
		GizmoManipulateMode mode = m_Gizmo->GetManipulateMode();
		return mode;
	}
	assert(false && "gizmo not set");
	return GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL;
}

bool KEEntityManipulator::SetManipulateMode(GizmoManipulateMode mode)
{
	if (m_Gizmo)
	{
		m_Gizmo->SetManipulateMode(mode);
		return true;
	}
	return false;
}

KEEntityPtr KEEntityManipulator::GetEntity(IKEntity::IDType id)
{
	auto it = m_Entities.find(id);
	if (it != m_Entities.end())
	{
		return it->second;
	}
	return nullptr;
}

KEEntityPtr KEEntityManipulator::CloestPickEntity(size_t x, size_t y)
{
	if (m_Scene && m_Camera)
	{
		IKEntity* entity;
		KEEntityPtr editorEntity;

		size_t width = 0;
		size_t height = 0;
		m_Window->GetInputSize(width, height);

		if (m_Scene->CloestPick(*m_Camera, x, y,
			width, height, entity) && (editorEntity = GetEditorEntity(entity)))
		{
			return editorEntity;
		}
	}
	return nullptr;
}