#include "KEEntityManipulateCommand.h"
#include "Manipulator/KEEntityManipulator.h"
#include "KEditorGlobal.h"

KEEntitySceneJoinCommand::KEEntitySceneJoinCommand(KEEntityPtr entity,
	IKScene* scene,
	KEEntityManipulator* manipulator)
	: m_Entity(entity),
	m_Scene(scene),
	m_Manipulator(manipulator)
{
	assert(m_Entity && m_Entity->soul);
}

KEEntitySceneJoinCommand::~KEEntitySceneJoinCommand()
{
}

void KEEntitySceneJoinCommand::Execute()
{
	if (KEditorGlobal::ResourceImporter.InitEntity(
		m_Entity->createInfo.path,
		m_Entity->soul))
	{
		m_Entity->soul->SetTransform(m_Entity->createInfo.transform);
		m_Scene->Add(m_Entity->soul);
		m_Manipulator->AddEditorEntity(m_Entity);
	}
}

void KEEntitySceneJoinCommand::Undo()
{
	KEditorGlobal::ResourceImporter.UnInitEntity(m_Entity->soul);
	m_Scene->Remove(m_Entity->soul);
	m_Manipulator->RemoveEditorEntity(m_Entity->soul->GetID());
}


KEEntitySceneEraseCommand::KEEntitySceneEraseCommand(KEEntityPtr entity,
	IKScene* scene,
	KEEntityManipulator* manipulator)
	: m_Entity(entity),
	m_Scene(scene),
	m_Manipulator(manipulator)
{
	assert(m_Entity && m_Entity->soul);
}

KEEntitySceneEraseCommand::~KEEntitySceneEraseCommand()
{
}

void KEEntitySceneEraseCommand::Execute()
{
	KEditorGlobal::ResourceImporter.UnInitEntity(m_Entity->soul);
	m_Scene->Remove(m_Entity->soul);
	m_Manipulator->RemoveEditorEntity(m_Entity->soul->GetID());
}

void KEEntitySceneEraseCommand::Undo()
{
	if (KEditorGlobal::ResourceImporter.InitEntity(
		m_Entity->createInfo.path,
		m_Entity->soul))
	{
		m_Entity->soul->SetTransform(m_Entity->createInfo.transform);
		m_Scene->Add(m_Entity->soul);
		m_Manipulator->AddEditorEntity(m_Entity);
	}
}

KEEntitySceneTransformCommand::KEEntitySceneTransformCommand(KEEntityPtr entity,
	IKScene* scene,
	KEEntityManipulator* manipulator,
	const glm::mat4& previous,
	const glm::mat4& current)
	: m_Entity(entity),
	m_Manipulator(manipulator),
	m_Scene(scene),
	m_Previous(previous),
	m_Current(current)
{
}

KEEntitySceneTransformCommand::~KEEntitySceneTransformCommand()
{
}

void KEEntitySceneTransformCommand::Execute()
{
	m_Entity->createInfo.transform = m_Current;
	m_Entity->soul->SetTransform(m_Current);
	m_Scene->Move(m_Entity->soul);
	// 更新一下Gizmo的位置
	m_Manipulator->UpdateGizmoTransform();
}

void KEEntitySceneTransformCommand::Undo()
{
	m_Entity->createInfo.transform = m_Previous;
	m_Entity->soul->SetTransform(m_Previous);
	m_Scene->Move(m_Entity->soul);
	// 更新一下Gizmo的位置
	m_Manipulator->UpdateGizmoTransform();
}