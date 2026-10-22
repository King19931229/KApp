#include "KDebugConsole.h"
#include "KBase/Publish/KConfig.h"
#include <stdio.h>
#include <algorithm>
#include <memory>
#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#endif

KDebugConsole::KDebugConsole()
	: m_Thread(nullptr)
	, m_Stop(false)
{
}

KDebugConsole::~KDebugConsole()
{
	UnInit();
}

void KDebugConsole::ThreadFunc()
{
#ifdef _WIN32
	std::unique_ptr<FILE, decltype(&fclose)> consoleInput(nullptr, fclose);
	DWORD consoleMode = 0;
	if (GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(stdin))), &consoleMode))
	{
		// Closing this handle also discards a canceled console read on reinitialization.
		FILE* input = nullptr;
		if (fopen_s(&input, "CONIN$", "r") != 0)
		{
			return;
		}
		consoleInput.reset(input);
	}
#endif
	while(!m_Stop.load())
	{
#ifdef _WIN32
		HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		if(consoleHandle != NULL)
		{
			SetConsoleTextAttribute(consoleHandle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		}
#endif
		printf(":>");
		fflush(stdout);
		char buffer[2048];
#ifdef _WIN32
		if (!fgets(buffer, sizeof(buffer) - 1, consoleInput ? consoleInput.get() : stdin))
		{
			break;
		}
#else
		size_t length = 0;
		while (length < sizeof(buffer) - 2 && !m_Stop.load())
		{
			pollfd input = { fileno(stdin), POLLIN, 0 };
			if (input.fd < 0)
			{
				return;
			}
			int result = poll(&input, 1, 50);
			if (result < 0 && errno == EINTR)
			{
				continue;
			}
			if (result < 0)
			{
				return;
			}
			if (result == 0)
			{
				continue;
			}
			char value;
			ssize_t readSize = read(input.fd, &value, 1);
			if (readSize < 0 && errno == EINTR)
			{
				continue;
			}
			if (readSize <= 0)
			{
				if (length == 0)
				{
					return;
				}
				break;
			}
			buffer[length++] = value;
			if (value == '\n')
			{
				break;
			}
		}
		buffer[length] = '\0';
#endif
		if (m_Stop.load())
		{
			break;
		}
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
		m_Stop.store(false);
		clearerr(stdin);
		m_Thread = KNEW std::thread(&KDebugConsole::ThreadFunc, this);
		return true;
	}
	return false;
}

bool KDebugConsole::UnInit()
{
	if(m_Thread)
	{
		m_Stop.store(true);
#ifdef _WIN32
		HANDLE threadHandle = m_Thread->native_handle();
		while (WaitForSingleObject(threadHandle, 10) == WAIT_TIMEOUT)
		{
			CancelSynchronousIo(threadHandle);
		}
#endif
		m_Thread->join();
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