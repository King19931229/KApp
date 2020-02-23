#pragma once
#include <memory>
#include <functional>
class KECommand
{
public:
	virtual ~KECommand() {}
	virtual void Execute() {}
	virtual void Undo() {}
};

class KELambdaCommand : public KECommand
{
	std::function<void()> m_ExecFunc;
	std::function<void()> m_UndoFunc;
public:
	KELambdaCommand(std::function<void()> exec,
		std::function<void()> undo)
		: m_ExecFunc(exec),
		m_UndoFunc(undo)
	{
	}
	
	void Execute() override
	{
		m_ExecFunc();
	}

	void Undo() override 
	{
		m_UndoFunc();
	}
};

typedef std::shared_ptr<KECommand> KECommandPtr;

namespace KECommandUnility
{
	static inline KECommandPtr CreateLambdaCommand(std::function<void()> exec, std::function<void()> undo)
	{
		return KECommandPtr(new KELambdaCommand(exec, undo));
	}
}