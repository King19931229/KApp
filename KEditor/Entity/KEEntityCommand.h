#pragma once
#include "Command/KECommand.h"
#include "KEditorConfig.h"
#include "KRender/Interface/IKRenderScene.h"
#include <initializer_list>

class KEEntityManipulator;
class IKScene;

class KEEntityGroupCommandBase : public KECommand
{
protected:
	std::vector<KEEntityPtr> m_Entites;
	IKScene* m_Scene;
public:
	KEEntityGroupCommandBase(KEEntityPtr entity, IKScene* scene)
		: m_Entites{ entity },
		m_Scene(scene)
	{
	}
	KEEntityGroupCommandBase(std::initializer_list<KEEntityPtr> entites, IKScene* scene)
		: m_Entites{ entites },
		m_Scene(scene)
	{
	}
	KEEntityGroupCommandBase(IKScene* scene)
		: m_Scene(scene)
	{
	}

	inline void Append(KEEntityPtr entity) { m_Entites.push_back(entity); }

	template<typename Func>
	void ForwardExecute(Func func)
	{
		for (auto it = m_Entites.begin(), itEnd = m_Entites.end();
			it != itEnd; ++it)
		{
			KEEntityPtr entity = *it;
			func(entity);
		}
	}

	template<typename Func>
	void BackwardExecute(Func func)
	{
		for (auto it = m_Entites.rbegin(), itEnd = m_Entites.rend();
			it != itEnd; ++it)
		{
			KEEntityPtr entity = *it;
			func(entity);
		}
	}
};

class KEEntitySceneJoinCommand : public KEEntityGroupCommandBase
{
protected:
	KEEntityManipulator* m_Manipulator;
public:
	KEEntitySceneJoinCommand(KEEntityPtr entity, IKScene* scene, KEEntityManipulator* manipulator);
	KEEntitySceneJoinCommand(std::initializer_list<KEEntityPtr> entites, IKScene* scene, KEEntityManipulator* manipulator);
	KEEntitySceneJoinCommand(IKScene* scene, KEEntityManipulator* manipulator);
	~KEEntitySceneJoinCommand();

	void Execute() override;
	void Undo() override;
};

class KEEntitySceneEraseCommand : public KEEntityGroupCommandBase
{
protected:
	KEEntityManipulator* m_Manipulator;
public:
	KEEntitySceneEraseCommand(KEEntityPtr entity, IKScene* scene, KEEntityManipulator* manipulator);
	KEEntitySceneEraseCommand(std::initializer_list<KEEntityPtr> entites, IKScene* scene, KEEntityManipulator* manipulator);
	KEEntitySceneEraseCommand(IKScene* scene, KEEntityManipulator* manipulator);
	~KEEntitySceneEraseCommand();

	void Execute() override;
	void Undo() override;
};

class KEEntitySceneTransformCommand : public KECommand
{
protected:
	IKScene* m_Scene;
	KEEntityManipulator* m_Manipulator;
	struct KEEntityInfo
	{
		KEEntityPtr entity;
		glm::mat4 previousTransform;
		glm::mat4 currentTransform;
	};
	std::vector<KEEntityInfo> m_Entites;
public:
	KEEntitySceneTransformCommand(IKScene* scene, KEEntityManipulator* manipulator);
	~KEEntitySceneTransformCommand();

	void Append(KEEntityPtr entity, const glm::mat4& previous, const glm::mat4 current);

	void Execute() override;
	void Undo() override;
};