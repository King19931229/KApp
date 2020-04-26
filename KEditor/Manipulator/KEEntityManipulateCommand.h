#pragma once
#include "Command/KECommand.h"
#include "KEditorConfig.h"
#include "KRender/Interface/IKRenderScene.h"

class KEEntityManipulator;

class KEEntitySceneJoinCommand : public KECommand
{
protected:
	KEEntityPtr m_Entity;
	IKScene* m_Scene;
	KEEntityManipulator* m_Manipulator;
public:
	KEEntitySceneJoinCommand(KEEntityPtr entity, IKScene* scene, KEEntityManipulator* manipulator);
	~KEEntitySceneJoinCommand();

	void Execute() override;
	void Undo() override;
};

class KEEntitySceneEraseCommand : public KECommand
{
protected:
	KEEntityPtr m_Entity;
	IKScene* m_Scene;
	KEEntityManipulator* m_Manipulator;
public:
	KEEntitySceneEraseCommand(KEEntityPtr entity, IKScene* scene, KEEntityManipulator* manipulator);
	~KEEntitySceneEraseCommand();

	void Execute() override;
	void Undo() override;
};

class KEEntitySceneTransformCommand : public KECommand
{
protected:
	KEEntityPtr m_Entity;
	IKScene* m_Scene;
	KEEntityManipulator* m_Manipulator;
	glm::mat4 m_Previous;
	glm::mat4 m_Current;
public:
	KEEntitySceneTransformCommand(KEEntityPtr entity, IKScene* scene,
		KEEntityManipulator* manipulator,
		const glm::mat4& previous,
		const glm::mat4& current);
	~KEEntitySceneTransformCommand();

	void Execute() override;
	void Undo() override;
};