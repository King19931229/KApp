#pragma once
#include "Interface/IKSocket.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#endif

class KSocketClient : public IKSocketClient
{
protected:
	WSADATA m_wsaData;
	sockaddr_in m_sockAddr;
	SOCKET m_socket;
public:
	KSocketClient();
	virtual ~KSocketClient();
	virtual bool StartUp();
	virtual bool EndUp();
	virtual bool Connect(const char* ip, unsigned short port);
	virtual bool Send(const char* data, int lenData);
	virtual bool Receive(char* buffer, int lenBuffer);
};