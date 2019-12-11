#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <functional>

class KDebugConsole
{
public:
	typedef std::function<void(const char*)> InputCallBackType;
protected:
	std::thread* m_Thread;
	std::mutex m_Lock;
	std::vector<InputCallBackType*> m_InputCallbacks;
public:
	KDebugConsole();
	~KDebugConsole();

	bool Init();
	bool UnInit();
	void ThreadFunc();

	bool AddCallback(InputCallBackType* callback);
	bool RemoveCallback(InputCallBackType* callback);
};