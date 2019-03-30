#pragma once

#include "Interface/IKConfig.h"
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET SOCKET_TYPE;
#else
typedef int SOCKET_TYPE;
#endif
#include <functional>

struct IKSocketClient
{
	virtual ~IKSocketClient() {}
	virtual bool StartUp() = 0;
	virtual bool EndUp() = 0;
	virtual bool Connect(const char* ip, unsigned short port) = 0;
	virtual bool Send(const char* data, int lenData) = 0;
	virtual bool Receive(char* buffer, int lenBuffer) = 0;
};
typedef std::shared_ptr<IKSocketClient> IKSocketClientPtr;
EXPORT_DLL IKSocketClientPtr GetSocketClient();

struct IKSocketServer
{
	virtual ~IKSocketServer() {}
	virtual bool StartUp() = 0;
	virtual bool EndUp() = 0;
	virtual bool Bind(const char* ip, unsigned short port) = 0;
	virtual bool Listen(int count) = 0;
	virtual bool AddAcceptCallback(std::function<void(SOCKET_TYPE)>* func) = 0;
	virtual bool RemoveAcceptCallback(std::function<void(SOCKET_TYPE)>* func) = 0;
	virtual bool Send(SOCKET_TYPE socket, const char* data, int lenData) = 0;
	virtual bool Receive(SOCKET_TYPE socket, char* buffer, int lenBuffer) = 0;
	virtual bool Run() = 0;
	virtual bool Stop() = 0;
};
typedef std::shared_ptr<IKSocketServer> IKSocketServerPtr;
EXPORT_DLL IKSocketServerPtr GetSocketServer();