#pragma once
#include "KBase/Interface/IKRunable.h"
#include "KBase/Publish/KSystem.h"
#include <string>
#include <thread>

class KRunableThread
{
protected:
	IKRunablePtr m_Runable;
	std::thread m_Thread;
	std::string m_Name;
public:
	KRunableThread(IKRunablePtr runable, const std::string& name)
		: m_Runable(runable)
		, m_Name(name)
	{
	}

	void StartUp()
	{
		m_Runable->StartUp();
		m_Thread = std::move(std::thread([this]
		{
			m_Runable->Run();
		}));
		KSystem::SetThreadName(m_Thread, m_Name);
	}

	void ShutDown()
	{
		m_Runable->ShutDown();
		m_Thread.join();
	}
};

typedef std::shared_ptr<KRunableThread> KRunableThreadPtr;