#pragma once
#include "KECommand.h"
#include <list>

class KECommandInvoker
{
protected:
	std::list<KECommandPtr> m_HistoryCommands;
	std::list<KECommandPtr>::iterator m_CurrentPos;
	void Discard();
public:
	KECommandInvoker();
	void Execute(KECommandPtr& command);
	void Undo();
	void Redo();
	void Clear();
};