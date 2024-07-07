#pragma once
#include <memory>
#include <functional>

struct IKTaskWork
{
	virtual ~IKTaskWork() {}
	virtual void DoWork() = 0;
	virtual void Abandon() {}
	virtual const char* GetDebugInfo() { return "PleaseAssignTaskWorkName"; }
};
typedef std::shared_ptr<IKTaskWork> IKTaskWorkPtr;

template<typename Task>
class KTaskWork : public IKTaskWork
{
protected:
	Task m_Task;
public:
	template <typename Arg0Type, typename... ArgTypes>
	KTaskWork(Arg0Type&& Arg0, ArgTypes&&... Args)
		: m_Task(std::forward<Arg0Type>(Arg0), std::forward<ArgTypes>(Args)...)
	{
	}

	void DoWork() override
	{
		m_Task.DoWork();
	}

	void Abandon() override
	{
		m_Task.Abandon();
	}

	const char* GetDebugInfo() override
	{
		return m_Task.GetDebugInfo();
	}
};

struct KEmptyTaskWork : public IKTaskWork
{
	KEmptyTaskWork() {}
	void DoWork() override {}
};

struct KLambdaTaskWork : public IKTaskWork
{
	std::function<void()> function;

	KLambdaTaskWork(std::function<void()> task)
	: function(task)
	{}

	void DoWork() override
	{
		function();
	}
};

IKTaskWorkPtr CreateTaskWork(IKTaskWork* work);