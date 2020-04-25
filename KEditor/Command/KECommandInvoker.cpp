#include "KECommandInvoker.h"
#include <assert.h>

KECommandInvoker::KECommandInvoker()
	: m_CurrentPos(m_HistoryCommands.end())
{
	assert(m_HistoryCommands.empty());
}

void KECommandInvoker::Execute(KECommandPtr& command)
{
	command->Execute();

	if (!m_bLock)
	{
		if (m_CurrentPos != m_HistoryCommands.end())
		{
			m_CurrentPos = m_HistoryCommands.erase(m_CurrentPos, m_HistoryCommands.end());
			assert(m_CurrentPos == m_HistoryCommands.end());
		}

		m_HistoryCommands.push_back(command);
		m_CurrentPos = m_HistoryCommands.end();
	}
}

void KECommandInvoker::Push(KECommandPtr& command)
{
	if (!m_bLock)
	{
		if (m_CurrentPos != m_HistoryCommands.end())
		{
			m_CurrentPos = m_HistoryCommands.erase(m_CurrentPos, m_HistoryCommands.end());
			assert(m_CurrentPos == m_HistoryCommands.end());
		}

		m_HistoryCommands.push_back(command);
		m_CurrentPos = m_HistoryCommands.end();
	}
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

KECommandInvokerLockGuard KECommandInvoker::CreateLockGurad()
{
	return KECommandInvokerLockGuard(this);
}

void KECommandInvoker::Lock()
{
	m_bLock = true;
}

void KECommandInvoker::UnLock()
{
	m_bLock = false;
}