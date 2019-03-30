#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Publish/KObjectPool.h"

#include "Interface/IKLog.h"
#include "Publish/KHashString.h"
#include "Publish/KDump.h"

#include "Interface/IKCodec.h"
#include "Interface/IKMemory.h"
IKLogPtr pLog;

#include "Interface/IKSocket.h"
int main()
{
	IKSocketServerPtr pSocketServer = GetSocketServer();
	pSocketServer->StartUp();
	pSocketServer->Bind("127.0.0.1", 10080);
	pSocketServer->Listen(20);
	auto cb = [&pSocketServer](SOCKET_TYPE socket)->void
	{
		char* data = "try to send a message to you";
		pSocketServer->Send(socket, data, (int)strlen(data) + 1);
	};
	std::function<void(SOCKET_TYPE)> func = cb;
	pSocketServer->AddAcceptCallback(&func);
	pSocketServer->Run();

	IKSocketClientPtr pSocketClient = GetSocketClient();
	pSocketClient->StartUp();
	pSocketClient->Connect("127.0.0.1", 10080);
	char buffer[256] = {0};
	pSocketClient->Receive(buffer, 256);
	puts(buffer);

	pSocketServer->Stop();
}