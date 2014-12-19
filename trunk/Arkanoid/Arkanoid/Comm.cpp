#include "comm.h"

#pragma comment(lib, "ws2_32.lib" )

Socket::Socket(BYTE revision, BYTE version): m_socket(INVALID_SOCKET)
{
	WSAStartup(MAKEWORD(revision,version), &wsaData);
	memset(&m_address, 0, sizeof(m_address));
}
Socket::~Socket()
{
	Disconnect();
	WSACleanup();
}
void Socket::Disconnect()
{
	if(m_socket!=INVALID_SOCKET)
    {
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		memset(&m_address, 0, sizeof(m_address));
    }
}
BOOL Socket::Listen(WORD port)
{
	Disconnect();
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	memset(&m_address, 0, sizeof(m_address));
	m_address.sin_family = AF_INET;
	m_address.sin_port = htons(port);
	m_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if(::bind(m_socket, (SOCKADDR*) &m_address, sizeof(m_address)) == SOCKET_ERROR)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	if(::listen(m_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	error=NO_ERROR;
	return TRUE;
}
BOOL Socket::Accept(Socket &sock)
{
	if( m_socket == INVALID_SOCKET)
		return FALSE;
	sock.Disconnect();
	int addrlen = sizeof(sock.m_address);
	sock.m_socket = ::accept(m_socket, (SOCKADDR*)&sock.m_address, &addrlen);
	if( m_socket == INVALID_SOCKET )
	{
		error=WSAGetLastError();
		return FALSE;
	}
	return TRUE;
}
BOOL Socket::Connect(const char *ip, WORD port)
{
	Disconnect();
	m_address.sin_family = AF_INET;
	m_address.sin_port = htons(port);
	if(!ip)
	{
		m_address.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else if(strcmp(ip,"localhost")==0)
	{
		m_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	else if(strcmp(ip,"broadcast")==0)
	{
		m_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	}
	else
	{
		ULONG tmp=inet_addr(ip);
		if(tmp==INADDR_NONE)
			return FALSE;
		m_address.sin_addr.s_addr = tmp;
	}
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	if(::connect(m_socket, (SOCKADDR*) &m_address, sizeof(m_address)) == SOCKET_ERROR)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	error=NO_ERROR;
	return TRUE;
}
BOOL Socket::Receive(void *buffer, int n)
{
	int bytes=::recv(m_socket, (char *)buffer, n, 0);
	if(bytes!=n)
		error=WSAGetLastError();
	else
		error=NO_ERROR;
	return !error;
}
BOOL Socket::Send(const void *buffer, int n)
{
	int bytes = ::send(m_socket, (const char *)buffer, n, 0);
	if(bytes!=n)
		error=WSAGetLastError();
	else
		error=NO_ERROR;
	return !error;
}