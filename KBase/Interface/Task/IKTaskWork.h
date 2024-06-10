#pragma once
#include <memory>
#include <functional>

struct IKTaskWork
{
	virtual ~IKTaskWork() {}
	virtual void DoWork() = 0;
	virtual void Abandon() {}
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
		m_Task();
	}
};

typedef KTaskWork<std::function<void()>> KLambdaTaskWork;
struct KEmptyTaskWork : public IKTaskWork
{
	virtual void DoWork() {}
};

IKTaskWorkPtr CreateTaskWork(IKTaskWork* work);