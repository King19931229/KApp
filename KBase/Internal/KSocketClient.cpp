#include "KSocketClient.h"

EXPORT_DLL IKSocketClientPtr GetSocketClient()
{
	return IKSocketClientPtr(KNEW KSocketClient());
}

KSocketClient::KSocketClient()
{
#ifdef _WIN32
	memset(&m_wsaData, 0, sizeof(m_wsaData));
#endif
	m_socket = SOCKET_ERROR;
	memset(&m_sockAddr, 0, sizeof(m_sockAddr));
}

KSocketClient::~KSocketClient()
{

}

bool KSocketClient::StartUp()
{
#ifdef _WIN32
	WSAStartup(MAKEWORD(2, 2), &m_wsaData);
#endif
	m_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(m_socket == SOCKET_ERROR)
		return false;
	return true;
}

bool KSocketClient::EndUp()
{
	if(m_socket != SOCKET_ERROR)
	{
#ifdef _WIN32
		closesocket(m_socket);
#else
		shutdown(m_socket, SHUT_RDWR);
#endif
	}
#ifdef _WIN32
	WSACleanup();
#endif
	return true;
}

bool KSocketClient::Connect(const char* ip, unsigned short port)
{
	if(m_socket != SOCKET_ERROR)
	{
		memset(&m_sockAddr, 0, sizeof(m_sockAddr));
		// IPV4
		m_sockAddr.sin_family = PF_INET;
		m_sockAddr.sin_port = htons(port);
		m_sockAddr.sin_addr.s_addr = inet_addr(ip);
#ifdef _WIN32
		int ret = connect(m_socket, (SOCKADDR*)&m_sockAddr, sizeof(SOCKADDR));
#else
		int ret = connect(m_socket, (const sockaddr*)&m_sockAddr, sizeof(sockaddr_in));
#endif
		if(ret == SOCKET_ERROR)
			return true;
	}
	return false;
}

bool KSocketClient::Send(const char* data, int lenData)
{
	if(m_socket != SOCKET_ERROR)
	{
		int ret = send(m_socket, data, lenData, NULL);
		if(ret != SOCKET_ERROR)
			return true;
	}
	return false;
}

bool KSocketClient::Receive(char* buffer, int lenBuffer)
{
	if(m_socket != SOCKET_ERROR)
	{
		int ret = recv(m_socket, buffer, lenBuffer, NULL);
		if(ret != SOCKET_ERROR)
			return true;
	}
	return false;
}