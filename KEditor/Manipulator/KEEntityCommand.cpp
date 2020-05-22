#include "KEEntityCommand.h"
#include "Manipulator/KEEntityManipulator.h"
#include "KEditorGlobal.h"


KEEntitySceneJoinCommand::KEEntitySceneJoinCommand(KEEntityPtr entity, IKScene* scene, KEEntityManipulator* manipulator)
	: KEEntityGroupCommandBase(entity, scene),
	m_Manipulator(manipulator)
{
}

KEEntitySceneJoinCommand::KEEntitySceneJoinCommand(std::initializer_list<KEEntityPtr> entites, IKScene* scene, KEEntityManipulator* manipulator)
	: KEEntityGroupCommandBase(entites, scene),
	m_Manipulator(manipulator)
{
}

KEEntitySceneJoinCommand::KEEntitySceneJoinCommand(IKScene* scene, KEEntityManipulator* manipulator)
	: KEEntityGroupCommandBase(scene),
	m_Manipulator(manipulator)
{
}

KEEntitySceneJoinCommand::~KEEntitySceneJoinCommand()
{
}

void KEEntitySceneJoinCommand::Execute()
{
	ForwardExecute([this](KEEntityPtr entity)
	{
		if (KEditorGlobal::ResourcePorter.InitEntity(
			entity->createInfo.path,
			entity->soul))
		{
			entity->soul->SetTransform(entity->createInfo.transform);
			m_Scene->Add(entity->soul);
			m_Manipulator->AddEditorEntity(entity);
		}
	});
}

void KEEntitySceneJoinCommand::Undo()
{
	BackwardExecute([this](KEEntityPtr entity)
	{
		if (KEditorGlobal::ResourcePorter.UnInitEntity(entity->soul))
		{
			m_Scene->Remove(entity->soul);
			m_Manipulator->RemoveEditorEntity(entity->soul->GetID());
		}
	});
}

KEEntitySceneEraseCommand::KEEntitySceneEraseCommand(KEEntityPtr entity, IKScene* scene, KEEntityManipulator* manipulator)
	: KEEntityGroupCommandBase(entity, scene),
	m_Manipulator(manipulator)
{
}

KEEntitySceneEraseCommand::KEEntitySceneEraseCommand(std::initializer_list<KEEntityPtr> entites, IKScene* scene, KEEntityManipulator* manipulator)
	: KEEntityGroupCommandBase(entites, scene),
	m_Manipulator(manipulator)
{
}

KEEntitySceneEraseCommand::KEEntitySceneEraseCommand(IKScene* scene, KEEntityManipulator* manipulator)
	: KEEntityGroupCommandBase(scene),
	m_Manipulator(manipulator)
{
}

KEEntitySceneEraseCommand::~KEEntitySceneEraseCommand()
{
}

void KEEntitySceneEraseCommand::Execute()
{
	ForwardExecute([this](KEEntityPtr entity)
	{
		if (KEditorGlobal::ResourcePorter.UnInitEntity(entity->soul))
		{
			m_Scene->Remove(entity->soul);
			m_Manipulator->RemoveEditorEntity(entity->soul->GetID());
		}
	});
}

void KEEntitySceneEraseCommand::Undo()
{
	BackwardExecute([this](KEEntityPtr entity)
	{
		if (KEditorGlobal::ResourcePorter.InitEntity(
			entity->createInfo.path,
			entity->soul))
		{
			entity->soul->SetTransform(entity->createInfo.transform);
			m_Scene->Add(entity->soul);
			m_Manipulator->AddEditorEntity(entity);
		}
	});
}

KEEntitySceneTransformCommand::KEEntitySceneTransformCommand(IKScene* scene,
	KEEntityManipulator* manipulator)
	: m_Manipulator(manipulator),
	m_Scene(scene)
{
}

KEEntitySceneTransformCommand::~KEEntitySceneTransformCommand()
{
}

void KEEntitySceneTransformCommand::Append(KEEntityPtr entity, const glm::mat4& previous, const glm::mat4 current)
{
	m_Entites.push_back(
	{
		entity,
		previous,
		current
	});
}

void KEEntitySceneTransformCommand::Execute()
{
	for (auto it = m_Entites.begin(), itEnd = m_Entites.end();
		it != itEnd; ++it)
	{
		KEEntityInfo& info = *it;
		info.entity->createInfo.transform = info.currentTransform;
		info.entity->soul->SetTransform(info.currentTransform);

		KReflectionObjectBase* reflection = nullptr;
		info.entity->soul->QueryReflection(&reflection);
		ASSERT_RESULT(reflection);
		KEditorGlobal::ReflectionManager.NotifyToProperty(reflection);

		m_Scene->Move(info.entity->soul);
		// 更新一下Gizmo的位置
		m_Manipulator->UpdateGizmoTransform();
	}
}

void KEEntitySceneTransformCommand::Undo()
{
	for (auto it = m_Entites.rbegin(), itEnd = m_Entites.rend();
		it != itEnd; ++it)
	{
		KEEntityInfo& info = *it;
		info.entity->createInfo.transform = info.previousTransform;
		info.entity->soul->SetTransform(info.previousTransform);

		KReflectionObjectBase* reflection = nullptr;
		info.entity->soul->QueryReflection(&reflection);
		ASSERT_RESULT(reflection);
		KEditorGlobal::ReflectionManager.NotifyToProperty(reflection);

		m_Scene->Move(info.entity->soul);
		// 更新一下Gizmo的位置
		m_Manipulator->UpdateGizmoTransform();
	}
}