#pragma once
#include "Command/KECommand.h"
#include "KEditorGlobal.h"
#include "Graph/Node/KEGraphNodeControl.h"
#include "Graph/KEGraphScene.h"

class KEGraphNodeCreateCommand : public KECommand
{
protected:
	KEGraphScene* m_Scene;
	KEGraphNodeControlPtr m_GraphNode;
public:
	KEGraphNodeCreateCommand(KEGraphScene* scene, KEGraphNodeControlPtr node)
		: m_Scene(scene),
		m_GraphNode(node)
	{
	}

	~KEGraphNodeCreateCommand()
	{
	}

	void Execute() override
	{
		m_GraphNode->Enter(m_Scene);
		m_Scene->m_Node.insert({ m_GraphNode->ID(), m_GraphNode });
	}

	void Undo() override
	{
		m_GraphNode->Exit(m_Scene);
		m_Scene->m_Node.erase(m_GraphNode->ID());
	}
};

class KEGraphNodeRemoveCommand : public KECommand
{
protected:
	KEGraphScene* m_Scene;
	KEGraphNodeControlPtr m_GraphNode;
public:
	KEGraphNodeRemoveCommand(KEGraphScene* scene, KEGraphNodeControlPtr node)
		: m_Scene(scene),
		m_GraphNode(node)
	{
	}

	~KEGraphNodeRemoveCommand()
	{
	}

	void Execute() override
	{
		m_GraphNode->Exit(m_Scene);
		m_Scene->m_Node.erase(m_GraphNode->ID());
	}

	void Undo() override
	{
		m_GraphNode->Enter(m_Scene);
		m_Scene->m_Node.insert({ m_GraphNode->ID(), m_GraphNode });
	}
};