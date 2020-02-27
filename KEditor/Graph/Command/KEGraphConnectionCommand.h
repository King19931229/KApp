#pragma once
#include "Command/KECommand.h"
#include "KEditorGlobal.h"
#include "Graph/Connection/KEGraphConnectionControl.h"
#include "Graph/KEGraphScene.h"

class KEGraphConnectionCreateCommand : public KECommand
{
protected:
	KEGraphScene* m_Scene;
	KEGraphConnectionControlPtr m_GraphConn;
public:
	KEGraphConnectionCreateCommand(KEGraphScene* scene, KEGraphConnectionControlPtr conn)
		: m_Scene(scene),
		m_GraphConn(conn)
	{
	}

	~KEGraphConnectionCreateCommand()
	{
	}

	void Execute() override
	{
		m_GraphConn->Enter(m_Scene);
		m_Scene->m_Connection[m_GraphConn->ID()] = m_GraphConn;
	}

	void Undo() override
	{
		m_GraphConn->Exit(m_Scene);
		m_Scene->m_Connection.erase(m_GraphConn->ID());
	}
};

class KEGraphConnectionRemoveCommand : public KECommand
{
protected:
	KEGraphScene* m_Scene;
	KEGraphConnectionControlPtr m_GraphConn;
public:
	KEGraphConnectionRemoveCommand(KEGraphScene* scene, KEGraphConnectionControlPtr node)
		: m_Scene(scene),
		m_GraphConn(node)
	{
	}

	~KEGraphConnectionRemoveCommand()
	{
	}

	void Execute() override
	{
		m_GraphConn->Exit(m_Scene);
		m_Scene->m_Connection.erase(m_GraphConn->ID());
	}

	void Undo() override
	{
		m_GraphConn->Enter(m_Scene);
		m_Scene->m_Connection[m_GraphConn->ID()] = m_GraphConn;
	}
};