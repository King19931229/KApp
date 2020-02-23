#pragma once
#include "KECommand.h"
#include <deque>

class KECommandInvoker
{
protected:
	std::deque<KECommandPtr> m_HistoryCommands;
	std::deque<KECommandPtr>::iterator m_CurrentPos;
	void Discard();
public:
	KECommandInvoker();
	void Execute(KECommandPtr& command);
	void Undo();
	void Redo();
	void Clear();
};