#include "KECommandInvoker.h"
#include <assert.h>

KECommandInvoker::KECommandInvoker()
	: m_CurrentPos(m_HistoryCommands.end())
{
}

void KECommandInvoker::Discard()
{
	if (m_CurrentPos != m_HistoryCommands.end())
	{
		m_CurrentPos = m_HistoryCommands.erase(m_CurrentPos, m_HistoryCommands.end());
		assert(m_CurrentPos == m_HistoryCommands.end());
	}
}

void KECommandInvoker::Execute(KECommandPtr& command)
{
	Discard();
	command->Execute();
	m_HistoryCommands.push_back(command);
	m_CurrentPos = m_HistoryCommands.end();
}

void KECommandInvoker::Undo()
{
	if (m_CurrentPos != m_HistoryCommands.begin())
	{
		--m_CurrentPos;
		(*m_CurrentPos)->Undo();
	}
}

void KECommandInvoker::Redo()
{
	if (m_CurrentPos != m_HistoryCommands.end())
	{
		(*m_CurrentPos)->Execute();
		++m_CurrentPos;
	}
}

void KECommandInvoker::Clear()
{
	m_HistoryCommands.clear();
	m_CurrentPos = m_HistoryCommands.end();
}