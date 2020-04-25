#pragma once
#include "KECommand.h"
#include <list>

class KECommandInvoker
{
	friend class KECommandInvokerLockGuard;
protected:
	std::list<KECommandPtr> m_HistoryCommands;
	std::list<KECommandPtr>::iterator m_CurrentPos;
	bool m_bLock;
public:
	KECommandInvoker();
	// push the command and execute
	void Execute(KECommandPtr& command);
	// push the command only
	void Push(KECommandPtr& command);
	void Undo();
	void Redo();
	void Clear();

	KECommandInvokerLockGuard CreateLockGurad();
	void Lock();
	void UnLock();
};

class KECommandInvokerLockGuard
{
protected:
	KECommandInvoker* m_Invoker;
	bool m_bIsLocked;
public:
	KECommandInvokerLockGuard(KECommandInvoker* invoker)
		: m_Invoker(invoker),
		m_bIsLocked(invoker->m_bLock)
	{
		invoker->m_bLock = true;
	}
	
	~KECommandInvokerLockGuard()
	{
		m_Invoker->m_bLock = m_bIsLocked;
	}
};