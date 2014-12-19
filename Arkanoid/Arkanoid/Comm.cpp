#include "comm.h"

#pragma comment(lib, "ws2_32.lib" )

Socket::Socket(BYTE revision, BYTE version):
	sock_connect(INVALID_SOCKET),
	sock_listen(INVALID_SOCKET)
{
	WSAStartup(MAKEWORD(revision,version), &wsaData);
}
Socket::~Socket()
{
	StopListen();
	WSACleanup();
}
void Socket::Disconnect()
{
	if(sock_connect!=INVALID_SOCKET)
    {
		closesocket(sock_connect);
		sock_connect = INVALID_SOCKET;
    }
}
void Socket::StopListen()
{
	Disconnect();
	if(sock_listen!=INVALID_SOCKET)
    {
		closesocket(sock_listen);
		sock_listen = INVALID_SOCKET;
    }
}
BOOL Socket::StartListen(WORD port)
{
	StopListen();
	sock_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_connect == INVALID_SOCKET)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	memset(&addr_listen, 0, sizeof(addr_listen));
	addr_listen.sin_family = AF_INET;
	addr_listen.sin_port = htons(port);
	addr_listen.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if(::bind(sock_listen, (SOCKADDR*) &addr_listen, sizeof(addr_listen)) == SOCKET_ERROR)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	if(::listen(sock_listen, SOMAXCONN) == SOCKET_ERROR)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	error=NO_ERROR;
	return TRUE;
}
BOOL Socket::Accept()
{
	if( sock_listen == INVALID_SOCKET)
		return FALSE;
	int addrlen = sizeof(addr_connect);
	sock_connect = ::accept(sock_listen, (SOCKADDR*) &addr_connect, &addrlen);
	if( sock_connect == INVALID_SOCKET )
	{
		error=WSAGetLastError();
		return FALSE;
	}
	return TRUE;
}
BOOL Socket::Connect(const char *ip, WORD port)
{
	StopListen();
	memset(&addr_connect, 0, sizeof(addr_connect));
	addr_connect.sin_family = AF_INET;
	addr_connect.sin_port = htons(port);
	if(!ip)
	{
		addr_connect.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else if(strcmp(ip,"localhost")==0)
	{
		addr_connect.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	else if(strcmp(ip,"broadcast")==0)
	{
		addr_connect.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	}
	else
	{
		ULONG tmp=inet_addr(ip);
		if(tmp==INADDR_NONE)
			return FALSE;
		addr_connect.sin_addr.s_addr = tmp;
	}
	sock_connect = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_connect == INVALID_SOCKET)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	if(::connect(sock_connect, (SOCKADDR*) &addr_connect, sizeof(addr_connect)) == SOCKET_ERROR)
	{
		error=WSAGetLastError();
		return FALSE;
	}
	error=NO_ERROR;
	return TRUE;
}
BOOL Socket::Receive(void *buffer, int n)
{
	int bytes=::recv(sock_connect, (char *)buffer, n, 0);
	if(bytes!=n)
		error=WSAGetLastError();
	else
		error=NO_ERROR;
	return !error;
}
BOOL Socket::Send(const void *buffer, int n)
{
	int bytes = ::send(sock_connect, (const char *)buffer, n, 0);
	if(bytes!=n)
		error=WSAGetLastError();
	else
		error=NO_ERROR;
	return !error;
}