#include "KSocketServer.h"
#include <thread>

EXPORT_DLL IKSocketServerPtr GetSocketServer()
{
	return IKSocketServerPtr(new KSocketServer());
}

KSocketServer::KSocketServer()
	: m_socket(SOCKET_ERROR),
	m_bRun(false)
{
	memset(&m_wsaData, 0, sizeof(m_wsaData));
	memset(&m_sockAddr, 0, sizeof(m_sockAddr));
}

KSocketServer::~KSocketServer()
{

}

bool KSocketServer::StartUp()
{
	WSAStartup(MAKEWORD(2, 2), &m_wsaData);
	m_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(m_socket == SOCKET_ERROR)
		return false;
	return true;
}

bool KSocketServer::EndUp()
{
	if(m_socket != SOCKET_ERROR)
		closesocket(m_socket);
	WSACleanup();
	return true;
}

bool KSocketServer::Bind(const char* ip, unsigned short port)
{
	if(m_socket != SOCKET_ERROR)
	{
		memset(&m_sockAddr, 0, sizeof(m_sockAddr));
		m_sockAddr.sin_family = PF_INET;
		m_sockAddr.sin_port = htons(port);
		m_sockAddr.sin_addr.s_addr = inet_addr(ip);

		int ret = bind(m_socket, (SOCKADDR*)&m_sockAddr, sizeof(SOCKADDR));
		if(ret != SOCKET_ERROR)
			return true;
	}
	return false;
}

void KSocketServer::ThreadFunc()
{
	int nSize = sizeof(SOCKADDR);
	int maxfdp = NULL;
	struct fd_set fds;
	struct timeval timeOut={0,1};
	while(m_bRun)
	{
		FD_ZERO(&fds);
		FD_SET(m_socket, &fds);
		if (select(maxfdp, &fds, NULL, NULL, &timeOut))
		{
			SOCKADDR m_sockClientAddr;
			SOCKET sockClient = accept(m_socket, (SOCKADDR*)&m_sockClientAddr, &nSize);
			if(!m_bRun)
				return;
			if(sockClient != SOCKET_ERROR)
			{
				std::lock_guard<decltype(m_cbLock)> lockGurad(m_cbLock);
				for(auto it = m_acceptCB.begin(), it_end = m_acceptCB.end(); it != it_end;
					++it)
				{
					auto pCallback = *it;
					if(pCallback != nullptr)
						(*pCallback)(sockClient);
				}
				closesocket(sockClient);
			}
		}
	}
}

bool KSocketServer::AddAcceptCallback(std::function<void(SOCKET_TYPE)>* func)
{
	std::lock_guard<decltype(m_cbLock)> lockGurad(m_cbLock);
	if(std::find(m_acceptCB.begin(), m_acceptCB.end(), func) == m_acceptCB.end())
	{
		m_acceptCB.push_back(func);
		return true;
	}
	return false;
}

bool KSocketServer::RemoveAcceptCallback(std::function<void(SOCKET_TYPE)>* func)
{
	std::lock_guard<decltype(m_cbLock)> lockGurad(m_cbLock);
	CallBackListType::iterator it = std::find(m_acceptCB.begin(), m_acceptCB.end(), func);
	if(it != m_acceptCB.end())
	{
		m_acceptCB.erase(it);
		return true;
	}
	return false;
}


bool KSocketServer::Listen(int count)
{
	int ret = listen(m_socket, count);
	if(ret == SOCKET_ERROR)
		return false;
	return true;
}

bool KSocketServer::Send(SOCKET_TYPE socket, const char* data, int lenData)
{
	if(socket != SOCKET_ERROR)
	{
		int ret = send(socket, data, lenData, NULL);
		if(ret != SOCKET_ERROR)
			return true;
	}
	return false;
}

bool KSocketServer::Receive(SOCKET_TYPE socket, char* buffer, int lenBuffer)
{
	if(socket != SOCKET_ERROR)
	{
		int ret = recv(socket, buffer, lenBuffer, NULL);
		if(ret != SOCKET_ERROR)
			return true;
	}
	return false;
}

bool KSocketServer::Run()
{
	m_bRun = true;
	m_runThread = std::thread(&KSocketServer::ThreadFunc, this);
	return true;
}

bool KSocketServer::Stop()
{
	m_bRun = false;
	m_runThread.join();
	return true;
}