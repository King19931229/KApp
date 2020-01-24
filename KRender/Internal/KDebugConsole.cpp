#include "KDebugConsole.h"
#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#endif

KDebugConsole::KDebugConsole()
	: m_Thread(nullptr)
{
}

KDebugConsole::~KDebugConsole()
{
}

void KDebugConsole::ThreadFunc()
{
	while(true)
	{
#ifdef _WIN32
		HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if(consoleHandle != NULL)
		{
			SetConsoleTextAttribute(consoleHandle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		}
#endif
		printf(":>");
		char buffer[2048];
		fgets(buffer, sizeof(buffer) - 1, stdin);
		{
			std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
			for(auto callback : m_InputCallbacks)
			{
				(*callback)(buffer);
			}
		}
	}
}

bool KDebugConsole::Init()
{
	if(!m_Thread)
	{
		m_Thread = new std::thread(&KDebugConsole::ThreadFunc, this);
		return true;
	}
	return false;
}

bool KDebugConsole::UnInit()
{
	if(m_Thread)
	{
		m_Thread->detach();
		delete m_Thread;
		m_Thread = nullptr;
		return true;
	}
	return false;
}

bool KDebugConsole::AddCallback(InputCallBackType* callback)
{
	if(callback)
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		auto it = std::find(m_InputCallbacks.begin(), m_InputCallbacks.end(), callback);
		if(it == m_InputCallbacks.end())
		{
			m_InputCallbacks.push_back(callback);
		}
		return true;
	}
	return false;
}

bool KDebugConsole::RemoveCallback(InputCallBackType* callback)
{
	if(callback)
	{
		std::lock_guard<decltype(m_Lock)> lockGuard(m_Lock);
		auto it = std::find(m_InputCallbacks.begin(), m_InputCallbacks.end(), callback);
		if(it != m_InputCallbacks.end())
		{
			m_InputCallbacks.erase(it);
		}
		return true;
	}
	return false;
}