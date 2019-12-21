#pragma once
#include "Interface/IKSocket.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#else
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#define SOCKET_ERROR -1
#endif

class KSocketClient : public IKSocketClient
{
protected:
#ifdef _WIN32
	WSADATA m_wsaData;
	SOCKET m_socket;
#else
	int m_socket;
#endif
	sockaddr_in m_sockAddr;
public:
	KSocketClient();
	virtual ~KSocketClient();
	virtual bool StartUp();
	virtual bool EndUp();
	virtual bool Connect(const char* ip, unsigned short port);
	virtual bool Send(const char* data, int lenData);
	virtual bool Receive(char* buffer, int lenBuffer);
};