#pragma once
#include "Interface/IKSocket.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#endif
#include <thread>
#include <mutex>
#include <vector>

class KSocketServer : public IKSocketServer
{
protected:
	WSADATA m_wsaData;
	sockaddr_in m_sockAddr;
	SOCKET m_socket;
	bool m_bRun;
	std::thread m_runThread;
	std::mutex m_cbLock;
	typedef std::vector<std::function<void(SOCKET_TYPE)>*> CallBackListType;
	CallBackListType m_acceptCB;
	void ThreadFunc();
public:
	KSocketServer();
	virtual ~KSocketServer();
	virtual bool StartUp();
	virtual bool EndUp();
	virtual bool Bind(const char* ip, unsigned short port);
	virtual bool Listen(int count);
	virtual bool AddAcceptCallback(std::function<void(SOCKET_TYPE)>* func);
	virtual bool RemoveAcceptCallback(std::function<void(SOCKET_TYPE)>* func);
	virtual bool Send(SOCKET_TYPE socket, const char* data, int lenData);
	virtual bool Receive(SOCKET_TYPE socket, char* buffer, int lenBuffer);
	virtual bool Run();
	virtual bool Stop();
};